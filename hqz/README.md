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

Scene Format
------------

The JSON input file is an object with a number of mandatory members:

* **"resolution"**: [ *width*, *height* ]  
	* Sets the output resolution, in pixels.
* **"viewport"**: [ *left*, *top*, *width*, *height* ]
	* Defines the scene coordinate system relative to the viewable area.
* **"lights"**: [ *light1*, *light2*, … ]
	* A list of all lights in the scene. (Details below)
* **"objects"**: [ *object1*, *object2*, … ]
	* A list of all objects in the scene. (Details below)

Optional members:

* **"seed"**: *integer*
	* Defines the 32-bit seed value for our pseudorandom number generator. Changing this value will change the specific pattern of noise in the rendering. By default this is arbitrarily set to zero.

### Sampled Values

Numeric values in your scene can be sampled stochastically every time they are evaluated. Any of these *sampled* parameters can be written either as a single JSON number or by using a convention to reference a particular distribution of random values.

* 1.0
	* A plain JSON number is a constant. Always returns 1.
* [ -2.8, 3 ]
	* A 2-element array of numbers is a uniformly distributed random variable. This is equally likely to return any number between -2.8 and 3.
* [ 6500, "K" ]
    * A special-purpose random variable for the wavelength of a photon emitted from an ideal blackbody source at the indicated color temperature in kelvins. 
* Etc.
	* Other values are reserved for future use, and currently evaluate to zero.	

### Light Format

Lights are each described as an array of sampled values:

* [0] **Light power**. The relative power of this light compared to other lights controls how often it casts rays relative to those other lights. Total power of all lights in a scene is factored into exposure calculations. Light power is linear.
* [1] **Cartesian X coordinate**. The base position of the light is an (x,y) coordinate. In viewport units.
* [2] **Cartesian Y coordinate**.
* [3] **Polar angle**. An additional polar coordinate can be added to the cartesian coordinate, to create round and arc shaped lights. In radians.
* [4] **Polar distance**. In viewport units.
* [5] **Wavelength**. In nanometers. Use a constant for monochromatic light, or use a blackbody random variable for full-spectrum light.

