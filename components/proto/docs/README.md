About
=====

The motion monitor protocol consists of a header, type, and payload.

The type is just a string that indicates what kind of information is encoded in the payload.

The header consists of two 4-byte unsigned integers (encoded as little endian).

The first integer is the number of characters of the type string.

The second integer is the number of bytes in the payload.

Messages
========

There are several topics used to exchange information between server and client.

The standardized ones are defined here.

## Server Messages

### image::update

Contains an image update from a video source.

The payload consists of the following fields:

| Field    | Type   | Description                             |
|==========|========|=========================================|
| width    | uint16 | The width of the image, in pixels.      |
| height   | uint16 | The height of the image, in pixels.     |
| channels | uint16 | The number of channels in the image.    |
| motion   | uint16 | A rating of how different the image is. |

After these fields is the pixel data of the image.

## Client Messages

## ready

Indicates that the client is ready for new data.
