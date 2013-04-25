High Quality Zen
================

A batch renderer in the style of Zen photon garden.

High Quality Zen (`hqz`) is a command line tool which converts a JSON scene description into a rendered PNG image. Rays are traced in 2D, just like on [zenphoton.com](http://zenphoton.com). But freed from the constraints of interactivity and HTML5, `hqz` can focus on creating high quality renderings for print and animation.

Artwork from [zenphoton.com](http://zenphoton.com) may be converted to JSON using an included Python script. This can be used to render print-quality versions of existing images, or as a starting point for experimenting with the other capabilities of `hqz`.

Planned features:

* Random variables as first-class objects. (Stochastically sample *anything*, render great motion blur.)
* Color rendering. Light rays model wavelength, lights and materials can specify spectra.
* Stopping condition based on a quality metric.
* Tools for dispatching video frames to an AWS compute cluster.
* Blender plugin for authoring animations.
