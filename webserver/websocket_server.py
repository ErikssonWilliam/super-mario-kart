#!/usr/bin/env python3
import asyncio
import websockets
import logging
import sys
import struct
from enum import IntEnum

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class ActionCode(IntEnum):
    """Action codes for client-server communication"""
    WELCOME = 0x01
    ECHO = 0x02
    PING = 0x03
    MOVE_UP = 0x04
    MOVE_DOWN = 0x05
    MOVE_LEFT = 0x06
    MOVE_RIGHT = 0x07
    ATTACK = 0x08
    DEFEND = 0x09
    STATUS_UPDATE = 0x0A
    ERROR = 0xFF

class MessageHandler:
    """Handles message encoding and decoding"""

    @staticmethod
    def encode_action(action_code: ActionCode) -> bytes:
        # Just one byte
        return struct.pack('!B', action_code)

    @staticmethod
    def decode_action(message: bytes) -> ActionCode:
        (action_code,) = struct.unpack('!B', message[:1])
        return ActionCode(action_code)

async def handle_client(websocket):
    client_address = websocket.remote_address
    logger.info(f"New client connected from {client_address}")

    try:
        # Send welcome message
        welcome_message = MessageHandler.encode_action(ActionCode.WELCOME)
        await websocket.send(welcome_message)
        logger.info(f"Sent welcome action to {client_address}")

        # Keep listening
        async for message in websocket:
            logger.info(f"Received raw: {message}")

            # If it's bytes, decode it
            if isinstance(message, bytes):
                try:
                    action = MessageHandler.decode_action(message)
                    logger.info(f"Decoded action: {action.name}")
                except Exception as e:
                    logger.error(f"Failed to decode action: {e}")
            else:
                logger.info(f"Received text message: {message}")

    except websockets.exceptions.ConnectionClosed:
        logger.info(f"Client {client_address} disconnected")
    except Exception as e:
        logger.error(f"Error handling client: {e}")
        import traceback
        traceback.print_exc()

async def main_async():
    host = "127.0.0.1"
    port = 8080
    
    logger.info(f"Python version: {sys.version}")
    try:
        logger.info(f"Websockets version: {websockets.__version__}")
    except:
        logger.info("Websockets version: unknown")
    
    logger.info(f"Starting Action-based WebSocket server on {host}:{port}")
    logger.info("Action codes supported:")
    for action in ActionCode:
        logger.info(f"  {action.name}: 0x{action:02X}")
    
    async with websockets.serve(handle_client, host, port):
        logger.info("WebSocket server is running. Press Ctrl+C to stop.")
        await asyncio.Future()
def main():
    try:
        asyncio.run(main_async())
    except KeyboardInterrupt:
        logger.info("Server stopped")

if __name__ == "__main__":
    main()