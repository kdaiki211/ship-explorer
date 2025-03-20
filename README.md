# Ship Explorer

## Overview

Ship Explorer is a system that uses an AI model based on SSD-Mobilenet v1 for object detection of ships and displays candidate ship names in bounding boxes based on AIS (Automatic Identification System) information at the time of video capture.

<img src="examples/ship_explorer/ship_explorer_screenshot.png">

This project is based on [dusty-nv/jetson-inference](https://github.com/dusty-nv/jetson-inference)'s [examples/detectnet](https://github.com/dusty-nv/jetson-inference/tree/master/examples/detectnet).

The AI model was trained using transfer learning on images of ships taken from a high vantage point (a building) overlooking the sea. After annotating them, the model was trained. AIS information is acquired from [aisstream.io](https://aisstream.io/).

## Features

- Fast object detection through TensorRT optimization of the model  
  - Sufficient frame rates (around 15 fps) for Full HD videos  
- Efficient overlay rendering using CUDA  
- Fusion of object detection and AIS information  
  - Using a projection matrix obtained from four points on a map (latitude, longitude) and their corresponding points on the video (x, y) to display the candidate ship positions on the video  
  - Displaying the name of the ship closest to the center of the bounding box (among the candidate ships) inside the bounding box of the detected ship  
- SSD-Mobilenet v1 model optimized for Ship Explorer  
  - Additional dataset used for transfer learning  

## Environment

- Computing environment  
  - Inference: NVIDIA Jetson TX1  
  - Training: NVIDIA GeForce RTX 4070 Ti, Intel Core i7-13700K, 32 GB RAM  
- Shooting environment (for dataset collection and inference)  
  - Equipment: Apple iPhone 16 Pro  
  - Resolution: 1920 x 1080  
  - Frame rate: 30 fps  
  - 5x optical zoom, 120mm, ƒ/2.8 aperture  
- Other  
  - Since the Jetson is installed in a remote location, the output video is viewed via GStreamer (with RTP, over VPN)  

## Project Structure

```
/jetson-inference/
├── build/                     # Compiled binaries
├── data/                      # Input data and samples
├── python/training/detection/ # Scripts for training (SSD-Mobilenet v1)
├── tools/                     # Tools for AIS information retrieval, RTP reception
└── examples/ship_explorer/    # Main implementation of this project
```

Usage of the source files contained in `examples/ship_explorer` is as follows:

* ship_explorer.cpp
  * Entry point for Ship Explorer
  * Reads AIS information from an NDJSON file
  * Loads video one frame at a time to perform object detection, display bounding boxes, and display ship names
* ais_loader.cpp
  * Expands AIS information from the NDJSON file into memory
* ais_stream_reader.cpp
  * Extracts ship information valid within the current time window from the AIS data in memory
* ais_bbox_mapper.cpp
  * Performs matching between bounding boxes and candidate ships included in the AIS information
  * Performs projection transformations

## Setup

### On Jetson TX1

#### JetPack

Install JetPack 4.6.6 ([the latest version that can be installed on TX1](https://developer.nvidia.com/embedded/jetpack-archive)) on the Jetson TX1 according to the steps in [Setting up Jetson with JetPack](https://github.com/dusty-nv/jetson-inference/blob/master/docs/jetpack-setup-2.md). Note that Jetson TX1 only supports installation via SDK Manager.

However, following the instructions as-is may lead to an installation failure due to insufficient eMMC capacity. As a workaround, first select only the minimum installation components to install JetPack to eMMC. Then copy the data from eMMC to the SD card, configure boot from the SD card, and run the JetPack installer again to install the previously skipped components.

For how to configure booting from an SD card, refer to [this article](https://jkjung-avt.github.io/sd-rootfs-on-tx1/).

#### Packages

```shell
$ sudo apt update
$ sudo apt install libpython3-dev python3-numpy git-lfs libyaml-cpp-dev
```

#### Build

```shell
$ cd /path/to/jetson-inference
$ mkdir build
$ cmake ..
$ make -j2
```

### On Windows or Ubuntu machine

#### GStreamer

Used for receiving and displaying the video stream from the Jetson TX1 in real time.  
On Windows, download and install both the runtime and development installers from the [official GStreamer website](https://gstreamer.freedesktop.org/download/#windows).

Example command:

```bat
> gst-launch-1.0.exe -v udpsrc port=1234 caps = "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96" ! rtph264depay ! decodebin ! videoconvert ! autovideosink
```

You can run the above command from jetson-inference/tools/gstreamer.bat.

#### Label Studio

Used for annotation.

```shell
$ pip install label-studio
```

## Video Shooting and AIS Information Retrieval

Ship Explorer requires that AIS information at the time of shooting the ship video is recorded beforehand and provided as input along with the video at runtime.

Before shooting the video for ship detection, edit jetson-inference/tools/get_ais.py, specify the aisstream.io API key as `APIKey`, and save it.  
Then, run the following command to save the AIS information to a log file:

```shell
$ cd /path/to/jetson-inference
$ cd tools
$ python3 -u get_ais.py | tee -a ais_log.ndjson
```

Because the transmission interval of AIS information differs for each ship, it is recommended to start acquiring and saving AIS information at least 10 minutes before shooting the video. While acquiring and saving AIS information, shoot the target video for ship detection. Note that the camera should not be moved during shooting, as the latitude/longitude to screen coordinate projection depends on a fixed camera view.

## Dataset Collection and Annotation

Several screenshots were taken from videos recorded at the same shooting location as the inference video, creating images for the dataset. These dataset images were then annotated using [Label Studio](https://labelstud.io/). For detailed instructions, refer to Label Studio’s [Quick start](https://labelstud.io/guide/quick_start) guide.

Only one class, `Boat`, was used, placing bounding boxes around ships in the images. After completing annotations for all images, export the annotation results in Pascal VOC XML format.

The Pascal VOC XML output directory from Label Studio does not include all files required by jetson-inference’s training/detection/ssd/train_ssd.py.

If the directory name exported from Label Studio is project-1-at-2025-03-10-16-37-70156aca, create any missing files or directories so that the structure matches what train_ssd.py expects:

- project-1-at-2025-03-10-16-37-70156aca  
  - Annotations/  
    - *.xml  
  - ImageSets/  
    - Main/  
      - train.txt  
      - val.txt  
      - test.txt  
      - trainval.txt  
  - JPEGImages/ (renamed from images)  
    - *.png  
  - labels.txt  

First, run the following command to get the basename of images in the dataset, then split them into training, validation, and test sets. In this case, 60% for training, 20% for validation, and 20% for testing. Create train.txt, val.txt, and test.txt with each basename separated by a newline, then create trainval.txt by concatenating train.txt and val.txt:

```shell
$ cd /path/to/output/dir
$ ls Annotations/ | sed -e 's/\.xml$//'
$ mkdir -p ImageSets/Main
$ cd ImageSets/Main
$ vim train.txt # List 60% of the dataset basenames
$ vim val.txt   # List 20% of the dataset basenames
$ vim test.txt  # List 20% of the dataset basenames
$ cat train.txt val.txt > trainval.txt
```

Rename the images directory to JPEGImages:

```shell
$ mv images JPEGImages
```

Create labels.txt containing the class name on a new line:

```shell
$ echo "Boat" > labels.txt
```

## Training

Use the annotated results created in the above steps to perform transfer learning on an SSD-Mobilenet v1 model by following [Re-training SSD-Mobilenet](https://github.com/dusty-nv/jetson-inference/blob/master/docs/pytorch-ssd.md).  
In this case, to reduce training time and process more epochs, the model was trained not on Jetson TX1 but on a Windows PC (x86_64) equipped with GeForce RTX 4070 Ti.

Specifically, run the following command. Specify the directory exported by Label Studio with `--data`. The output directory for the trained model (.pth) is specified with `--model-dir`. Here, the output directory is `models/ddsn_boat7_using_my_dataset`.  
In this example, a total of 500 epochs were trained:

```shell
$ # on Windows PC (x86_64)
$ cd /path/to/jetson-inference
$ cd python/training/detection/ssd
$ python3 train_ssd.py --dataset-type=voc --data=data/project-1-at-2025-03-10-16-37-70156aca --epochs=500 --batch-size=64 --model-dir=models/ddsn_boat7_using_my_dataset
```

After execution completes, a file with a name like `mb1-ssd-Epoch-499-Loss-2.1141648292541504.pth` will be output to the directory specified by `--model-dir`.  
Although the model output by train_ssd.py is in PyTorch format (.pth), the detectnet-based code used in Ship Explorer requires an ONNX model (.onnx). Convert the model to ONNX format by following [Converting the Model to ONNX](https://github.com/dusty-nv/jetson-inference/blob/master/docs/pytorch-ssd.md#converting-the-model-to-onnx).

Finally, transfer the model file output by onnx_export.py (mb1-ssd-Epoch-499-Loss-2.1141648292541504.onnx) to `jetson-inference/python/training/detection/ssd/models/` on the Jetson TX1.

## Execution

The Ship Explorer executable is generated at build/aarch64/bin/ship_explorer.  
When executed, it sends the video stream from the Jetson TX1 to the specified RTP address.

For example, run ship_explorer as shown below. If you use a VSCode environment, you can execute it more easily using .vscode/launch.json.

```shell
$ cd /path/to/jetson-inference
$ cd build/aarch64/bin/
$ ship_explorer --model python/training/detection/ssd/models/ddsn_boat7_using_my_dataset/mb1-ssd-Epoch-499-Loss-2.1141648292541504.onnx \
                --labels python/training/detection/ssd/models/ddsn_boat7_using_my_dataset/labels.txt \
                --input-blob input_0 \
                --output-cvg scores \
                --output-bbox boxes \
                --threshold 0.2 \
                --tracking \
                --tracker-min-frames 40 \
                --tracker-overlap 0.7 \
                --overlay box,shipname,debuginfo1 \
                --config data/ship_explorer/IMG_8070.yaml \
                /home/nvidia/jetson-inference/data/ship_explorer/IMG_8070.MOV \
                rtp://192.168.62.97:1234
```

The arguments follow those in examples/detectnet.cpp, but Ship Explorer adds the following options. Specify them as shown:

- `--overlay` includes `shipname`, `debuginfo1`, `debuginfo2` (`debuginfo1` and `debuginfo2` are optional)  
  - `shipname`  
    - Displays the ship name, obtained by matching AIS information, inside the bounding box of the detected ship  
    - If no candidate ship name is found from AIS information, it displays "?"  
  - `debuginfo1`  
    - Displays points a, b, c, d used for the latitude/longitude → screen coordinate projection, and the ship position information obtained from AIS, on the screen  
  - `debuginfo2`  
    - Displays AIS information as text on the screen  
- `--config` specifies a YAML file containing parameters related to the input video  
  Example:
  ```yaml
  # geo to screen mapping for perspective transform
  src-geo:
    a: [35.592499, 139.790526]
    b: [35.586024, 139.784049]
    c: [35.633655, 139.759223]
    d: [35.625142, 139.767833]
  dst-scr:
    a: [682, 364]
    b: [1672, 355]
    c: [733, 905]
    d: [217, 597]
  
  # AIS log file (ndjson format)
  ais-ndjson: "data/ship_explorer/ais_log_20250308_152449.ndjson"
  
  # input video file and its recorded timestamp
  input-timestamp-utc: "2025-03-08 23:16:57"
  ```
- `src-geo`, `dst-scr` specify the four points used for the latitude/longitude → screen coordinate projection  
  - `src-geo` contains latitude and longitude  
  - `dst-scr` contains the corresponding x, y coordinates on the screen (video)  
- `aid-ndjson` specifies the filename of the JSON data (newline-delimited) obtained from aisstream.io  
- `input-timestamp-utc` specifies the shooting start time of the video in UTC  

The sample input videos used for the operation check have been uploaded to YouTube.

* [IMG_8070.MOV](https://youtu.be/EMo_rdJeIP0)
* [IMG_8071.MOV](https://youtu.be/7LHS-8FcL8Q)
* [IMG_8072.MOV](https://youtu.be/EFFWqWWuif8)

## Future Issues and Outlook

- Improving model accuracy  
  - Currently, the accuracy of ship detection is low, possibly due to insufficient dataset size  
  - The model may lose track of a ship or recognize one ship as two or more  
  - A larger dataset is needed to improve accuracy  
    - Various times of day  
    - Various types and sizes of ships  
    - Inclement weather or nighttime scenes  
- Real-time detection  
  - Use a high-magnification (5x or more) USB camera and real-time AIS information to display ship names  
    (Currently, the system requires pre-recorded videos and separately saved AIS information in NDJSON format at the time of shooting)  
- Possible applications of this project  
  - Tourism  
    - Displaying names of ships moving on the sea in real time  
  - Maritime monitoring  
    - Aiding ship identification to enhance safety management  
    - Sending alerts if a ship not transmitting AIS is found  

## License

This follows the license of the fork source [dusty-nv/jetson-inference](https://github.com/dusty-nv/jetson-inference).  
See LICENSE.md for details.