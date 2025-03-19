import asyncio
import websockets
import json
from datetime import datetime, timezone

async def connect_ais_stream():

    async with websockets.connect("wss://stream.aisstream.io/v0/stream") as websocket:
        subscribe_message = {"APIKey": "<YOUR-API-KEY-HERE>",  # Required !
                             "BoundingBoxes": [[[35.67135034672569, 139.7410558663665], [35.57745014189914, 139.8416293027593]]]} # Required!

        subscribe_message_json = json.dumps(subscribe_message)
        await websocket.send(subscribe_message_json)

        async for message_json in websocket:
            print(message_json)

if __name__ == "__main__":
    asyncio.run(asyncio.run(connect_ais_stream()))
