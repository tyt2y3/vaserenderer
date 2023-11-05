# Vaser Unity

Vaser Unity is a continuation of [VASE renderer](https://github.com/tyt2y3/vaserenderer), reimplementing the original C++ algorithm in C# while taking advantage of fragment shader to reduce triangle count. A major difference is that "feathering" no longer requires extra vertices. So the following patch requires only 4 vertices (instead of 8 in the C++ version):

![Screenshort](Docs/Screenshot%20at%202019-12-02%2000-44-00.png)

![Screenshot](Docs/Screenshot%20at%202019-12-11%2001-11-10.png)
![Screenshot](Docs/Screenshot%20at%202019-12-15%2003-31-00.png)
![Screenshot](Docs/Screenshot%20at%202019-12-30%2017-36-00.png)

## Features

- Per vertex coloring and weighting
- Linear gradient along curve
- Brush-like feathering effects
- Premium quality anti-aliasing
- Various caps and joints

More screenshots can be found under [Docs](Docs).

## Status

Current implementation is still experimental, may be buggy and the API is subject to change. While this library can be used standalone, it may eventually be integrated into a larger framework.
