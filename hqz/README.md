High Quality Zen
================

A batch renderer in the style of Zen photon garden.

High Quality Zen (`hqz`) is a command line tool which converts a JSON scene description into a rendered PNG image. Rays are traced in 2D, just like on [zenphoton.com](http://zenphoton.com). But freed from the constraints of interactivity and HTML5, `hqz` can focus on creating high quality renderings for print and animation.

Artwork from [zenphoton.com](http://zenphoton.com) may be converted to JSON using an included zen2json script. This can be used to render print-quality versions of existing images, or as a starting point for experimenting with the other capabilities of `hqz`.


Build
-----

Batteries included: `hqz` has no external dependencies aside from the C++ standard library.

On Mac OS X, an XCode project file is included. For other platforms, there's a generic Makefile:

	$ make
	cc -c -o src/zrender.o src/zrender.cpp -Isrc -Wall -Werror -g -O3 -ffast-math -fno-exceptions
	…
	$ ./hqz example.json example.png
	$ open example.png

Scene Format
------------

The JSON input file is an object with a number of mandatory members:

* **"resolution"**: [ *width*, *height* ]  
	* Sets the output resolution, in pixels.
* **"viewport"**: [ *left*, *top*, *width*, *height* ]
	* Defines the scene coordinate system relative to the viewable area.
* **"lights"**: [ *light 0*, *light 1*, … ]
	* A list of all lights in the scene. (Details below)
* **"objects"**: [ *object 0*, *object 1*, … ]
	* A list of all objects in the scene. (Details below)
* **"materials"**: [ *material 0*, *material 1*, … ]
	* A list of all materials in the scene. (Details below)
* **"exposure"**: *float*
    * Sets the exposure (brightness) of the rendering. Units are an arbitrary logarithmic scale which matches [zenphoton.com](http://zenphoton.com)'s exposure slider over the range [0,1].
* **"rays"**: *integer*
    * Number of rays to cast. Note that some rays may be outside the visible range if you're using a blackbody random varaible for your light wavelength, so this number may need to be higher than otherwise.

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

A light is a thing that can emit rays. Lights are each described as an array of sampled values:

* [0] **Light power**. The relative power of this light compared to other lights controls how often it casts rays relative to those other lights. Total power of all lights in a scene is factored into exposure calculations. Light power is linear.
* [1] **Cartesian X coordinate**. The base position of the light is an (x,y) coordinate. In viewport units.
* [2] **Cartesian Y coordinate**.
* [3] **Polar angle**. An additional polar coordinate can be added to the cartesian coordinate, to create round and arc shaped lights. In degrees.
* [4] **Polar distance**. In viewport units.
* [5] **Ray angle**. In degrees.
* [6] **Wavelength**. In nanometers. Use a constant for monochromatic light, or use a blackbody random variable for full-spectrum light. Zero is a special pseudo-wavelength for monochromatic white light.

### Object Format

Scene objects are things that interact with rays once they've been emitted. Various kinds of objects are supported:

* [ *material*, *x0*, *y0*, *dx*, *dy* ]
	* A line segment, extending from (x0, y0) to (x0 + dx, y0 + dy). Coordinates are all sampled.
* Etc.
	* Other values are reserved for future use.

### Material Format

To reduce redundancy in the scene format, materials are referenced elsewhere by their zero-based array index in a global materials array. In `hqz`, a *material* specifies what happens to a ray after it comes into contact with an object.

A material is an array of weighted probabilistic outcomes. For example:

	[ [0.5, "d"], [0.2, "t"], [0.1, "r"] ]
	
This gives any incident rays a 50% chance of bouncing in a random direction (diffuse), a 20% chance of passing straight through (transmissive), a 10% chance of reflecting, and the remaining 20% chance is that the ray will be absorbed by the material. The empty array is a valid material, indicating that rays are always absorbed.

Various outcomes are supported, each identified by different kinds of JSON objects appended to the outcome's weight:

* [ *probability*, "d" ]
	* Perfectly diffuse reflection. Any incident rays bounce out at a random angle.
* [ *probability*, "t" ]
	* Ray is transmitted through the material without change.
* [ *probability*, "r" ]
	* Ray is reflected off of the object.
* Etc.
	* Other values are reserved for future use.
