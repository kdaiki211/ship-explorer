/*
 * Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "videoSource.h"
#include "videoOutput.h"

#include "detectNet.h"
#include "objectTracker.h"
#include "cudaDraw.h"
#include "cudaMath.h"
#include "cudaFont.h"

#include "ais_loader.hpp"
#include "ais_stream_reader.hpp"
#include "ais_bbox_mapper.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <signal.h>
#include <cassert>


bool signal_recieved = false;

void sig_handler(int signo)
{
	if( signo == SIGINT )
	{
		LogVerbose("received SIGINT\n");
		signal_recieved = true;
	}
}

int usage()
{
	printf("usage: ship_explorer [--help] [--network=NETWORK] [--threshold=THRESHOLD] ...\n");
	printf("                 input [output]\n\n");
	printf("Locate objects in a video/image stream using an object detection DNN.\n");
	printf("See below for additional arguments that may not be shown above.\n\n");
	printf("positional arguments:\n");
	printf("    input           resource URI of input stream  (see videoSource below)\n");
	printf("    output          resource URI of output stream (see videoOutput below)\n\n");

	printf("%s", detectNet::Usage());
	printf("%s", objectTracker::Usage());
	printf("%s", videoSource::Usage());
	printf("%s", videoOutput::Usage());
	printf("%s", Log::Usage());

	return 0;
}


int main( int argc, char** argv )
{
	/*
	 * parse command line
	 */
	commandLine cmdLine(argc, argv);

	if( cmdLine.GetFlag("help") )
		return usage();

	/*
	 * load AIS information
	 */
	AisLoader aisLoader;
	auto aisNdjsonFilename = cmdLine.GetString("ais-ndjson");
	if (aisNdjsonFilename) {
		LogVerbose("Loading AIS from %s...\n", aisNdjsonFilename);
		aisLoader = AisLoader(std::string(aisNdjsonFilename));
	}

	/*
	 * read the timestamp of the input video
	 */
	auto tsUtcStr = cmdLine.GetString("input-timestamp-utc");
	tm originTm = {};
	if (tsUtcStr) {
		std::istringstream ss(tsUtcStr);
		ss >> std::get_time(&originTm, "%Y-%m-%d %H:%M:%S");
	}

	/*
	 * attach signal handler
	 */
	if( signal(SIGINT, sig_handler) == SIG_ERR )
		LogError("can't catch SIGINT\n");


	/*
	 * create input stream
	 */
	videoSource* input = videoSource::Create(cmdLine, ARG_POSITION(0));

	if( !input )
	{
		LogError("detectnet:  failed to create input stream\n");
		return 1;
	}
	auto w = input->GetWidth();
	auto h = input->GetHeight();


	/*
	 * prepare AisStreamReader and AisBboxMapper
	 */
	assert(aisLoader.IsLoaded());
	AisStreamReader asr(aisLoader.GetLoadedShipInfo());
	LogVerbose("Loaded AIS successfully (%zu entries).\n", aisLoader.GetLoadedEntryCount());

	AisUtil::GeoCoords srcGeoPoints[4];
	cv::Point2f dstScreenPoints[4];
	const char* srcGeoPointsStr[] = {
		cmdLine.GetString("src-geo-a"),
		cmdLine.GetString("src-geo-b"),
		cmdLine.GetString("src-geo-c"),
		cmdLine.GetString("src-geo-d"),
	};
	const char* dstScreenPointsStr[] = {
		cmdLine.GetString("dst-scr-a"),
		cmdLine.GetString("dst-scr-b"),
		cmdLine.GetString("dst-scr-c"),
		cmdLine.GetString("dst-scr-d"),
	};
	for (int i = 0; i < 4; i++) {
		sscanf(srcGeoPointsStr[i], "%lf,%lf",
			&srcGeoPoints[i].latitude,
			&srcGeoPoints[i].longitude);
		int x, y;
		sscanf(dstScreenPointsStr[i], "%d,%d", &x, &y);
		dstScreenPoints[i].x = static_cast<float>(x);
		dstScreenPoints[i].y = static_cast<float>(y);
	}
	AisBboxMapper mapper(srcGeoPoints, dstScreenPoints);


	/*
	 * create output stream
	 */
	videoOutput* output = videoOutput::Create(cmdLine, ARG_POSITION(1));
	
	if( !output )
	{
		LogError("detectnet:  failed to create output stream\n");	
		return 1;
	}
	

	/*
	 * create detection network
	 */
	detectNet* net = detectNet::Create(cmdLine);
	
	if( !net )
	{
		LogError("detectnet:  failed to load detectNet model\n");
		return 1;
	}

	// parse overlay flags
	const uint32_t overlayFlags = detectNet::OverlayFlagsFromStr(cmdLine.GetString("overlay", "box,shipname"));
	
	// suppress logs
	Log::SetLevel(Log::Level::ERROR);

	/*
	 * processing loop
	 */
	while( !signal_recieved )
	{
		// capture next image
		uchar3* image = NULL;
		int status = 0;
		
		if( !input->Capture(&image, &status) )
		{
			if( status == videoSource::TIMEOUT )
				continue;
			
			break; // EOS
		}
		const int timestampInSec = input->GetFrameCount() / input->GetFrameRate();

		// update time window
		tm tm = originTm;
		AisUtil::AddSeconds(tm, timestampInSec);
		auto unixTime = mktime(&tm);
		asr.Update(unixTime);
		mapper.UpdateLocalShipInfo(unixTime, asr.GetCurrentWindow());

		// prepare debug info
		std::stringstream ss;
		// asr.PrintCurrentWindowSummary(&ss);
		mapper.PrintCurrentLocalShipInfoSummary(&ss);
		auto dbgInfo = ss.str();

		// draw debug info
		cudaFont* font = cudaFont::Create(adaptFontSize(w)); assert(font);
		auto format = IMAGE_RGB8;
		assert(font);
		if (overlayFlags & detectNet::OVERLAY_DEBUG_INFO) {
			float4 color = make_float4(0,0,255,255);

			// timestamp
			if (timestampInSec >= 0) {
				char tsStr[8];
				const int2 tsPos = make_int2(10, 10);
				sprintf(tsStr, "%d", timestampInSec);
				font->OverlayText(image, format, w, h, tsStr, tsPos.x, tsPos.y, color);
			}

			// debug info
			const int2 diPos = make_int2(w / 5, 10);
			font->OverlayText(image, format, w, h, dbgInfo.c_str(), diPos.x, diPos.y, color);
		}

		// detect objects in the frame
		detectNet::Detection* detections = NULL;
	
		const int numDetections = net->Detect(image, w, h, &detections, overlayFlags);
		
		if( numDetections > 0 )
		{
			LogVerbose("%i ship(s) detected\n", numDetections);

			auto shipNameList = mapper.SearchForShipName(detections, numDetections);
			for (auto shipName : shipNameList) {
				std::cout << "[" << shipName << "]" << std::endl;
			}
			std::cout << "---" << std::endl;

			// draw ship candidate position
			auto shipInfo  = mapper.GetLocalShipInfo();
			auto scrCoords = mapper.GetLocalShipScreenCoords();
			int shipIdx = 0;
			for (int i = 0; i < shipInfo.size(); i++) {
				auto str = shipInfo[i].shipName.c_str();
				auto elm = scrCoords[i];
				elm.x = min(max(elm.x, -1), w);
				elm.y = min(max(elm.y, -1), h);
				auto ext = font->TextExtents(str, elm.x, elm.y);
				auto pad = 5.0f;
				auto ox = (ext.z >= w-1) ? (ext.z - (w-1) + pad) : -pad;
				auto oy = (ext.w >= h-1) ? (ext.w - (h-1) + pad) : -pad;
				auto color = make_float4(0, 0, 0, 175.0f);
				if (elm.x < 0 || elm.y < 0 || elm.x >= w || elm.y >= h) {
					color.w = 70.0f;
				}
				auto r = 5.0f;
				CUDA(cudaDrawCircle(image, w, h, format, elm.x, elm.y, r, color));
				font->OverlayText(image, format, w, h, str, elm.x - ox, elm.y - oy, color);
			}

#if 1
			// draw calibration points (debug)
			AisUtil::ScreenCoords dstScreenPointsForVerify[] = {
				mapper.ConvertGeoCoordsToScreenCoords(srcGeoPoints[0]),
				mapper.ConvertGeoCoordsToScreenCoords(srcGeoPoints[1]),
				mapper.ConvertGeoCoordsToScreenCoords(srcGeoPoints[2]),
				mapper.ConvertGeoCoordsToScreenCoords(srcGeoPoints[3]),
			};
			for (int i = 0; i < 4; i++) {
				char str[2];
				str[0] = 'a' + i;
				str[1] = '\0';
				auto color = make_float4(255, 0, 0, 175);
				auto r = 5.0f;
				auto p = dstScreenPointsForVerify[i];
				CUDA(cudaDrawCircle(image, w, h, format, p.x, p.y, r, color));
				font->OverlayText(image, format, w, h, str, p.x + r + 1.0f, p.y, color);
			}
#endif
		
			for( int n=0; n < numDetections; n++ )
			{
				LogVerbose("\ndetected obj %i  class #%u (%s)  confidence=%f\n", n, detections[n].ClassID, net->GetClassDesc(detections[n].ClassID), detections[n].Confidence);
				LogVerbose("bounding box %i  (%.2f, %.2f)  (%.2f, %.2f)  w=%.2f  h=%.2f\n", n, detections[n].Left, detections[n].Top, detections[n].Right, detections[n].Bottom, detections[n].Width(), detections[n].Height()); 
			
				if( detections[n].TrackID >= 0 ) // is this a tracked object?
					LogVerbose("tracking  ID %i  status=%i  frames=%i  lost=%i\n", detections[n].TrackID, detections[n].TrackStatus, detections[n].TrackFrames, detections[n].TrackLost);

				if (overlayFlags & detectNet::OVERLAY_SHIPNAME) {
					std::string txt = "unknown";
					auto color = make_float4(255.0f, 255.0f, 255.0f, 255.0f);
					if (n < shipNameList.size()) {
						txt = shipNameList[n];
					}
					font->OverlayText(image, format, w, h, txt.c_str(), detections[n].Left, detections[n].Top, color);
				}
			}
		}	

		// render outputs
		if( output != NULL )
		{
			output->Render(image, input->GetWidth(), input->GetHeight());

			// update the status bar
			char str[256];
			sprintf(str, "TensorRT %i.%i.%i | %s | Network %.0f FPS", NV_TENSORRT_MAJOR, NV_TENSORRT_MINOR, NV_TENSORRT_PATCH, precisionTypeToStr(net->GetPrecision()), net->GetNetworkFPS());
			output->SetStatus(str);

			// check if the user quit
			if( !output->IsStreaming() )
				break;
		}

		// print out timing info
		// net->PrintProfilerTimes();
	}
	

	/*
	 * destroy resources
	 */
	LogVerbose("detectnet:  shutting down...\n");
	
	SAFE_DELETE(input);
	SAFE_DELETE(output);
	SAFE_DELETE(net);

	LogVerbose("detectnet:  shutdown complete.\n");
	return 0;
}

