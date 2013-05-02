High Quality Zen
================

A batch renderer in the style of Zen photon garden.

High Quality Zen (`hqz`) is a command line tool which converts a JSON scene description into a rendered PNG image. Rays are traced in 2D, just like on [zenphoton.com](http://zenphoton.com). But freed from the constraints of interactivity and HTML5, `hqz` can focus on creating high quality renderings for print and animation.

Artwork from [zenphoton.com](http://zenphoton.com) may be converted to JSON using an included `zen2json.coffee` script. This can be used to render print-quality versions of existing images, or as a starting point for experimenting with the other capabilities of `hqz`.

In `hqz` we extend the [zenphoton.com](http://zenphoton.com) scene model with many new features:

* Multiple light sources!
* Control over light source shape, angle, and **color**.
* Gamma correction and a 192-bit per pixel sample buffer for very high dynamic range. 
* Generic random variable system, for adding extra randomness anywhere.
* Rendering animations on the cloud with Amazon EC2.
* Extensible architecture, easy to use for generative animation.


Build for Local Rendering
-------------------------

If you want to run `hqz` on your own computer, you'll need to compile it from C++ source first. Fortunately, you won't need any dependencies other than a C++ compiler and the standard library.

You can build it by just running `make` on the command line. This should work on Mac OS X and Linux. Not tested on Windows, but maybe it will work anyway!

	$ make
	cc -c -o src/zrender.o src/zrender.cpp -Isrc -Wall -Werror -g -O3 -ffast-math -fno-exceptions
	…
	$ ./hqz example.json example.png
	$ open example.png
	

Node.js
-------

To use the cluster rendering features or the `zen2json` converter, you'll need [Node.js](http://nodejs.org). These scripts are written in [CoffeeScript](http://coffeescript.org) which is a pretty neat language that I hope you won't mind installing.

Once you have Node.js, you can use its [npm](https://npmjs.org/) package manager to install the other dependencies:

	npm install aws-sdk coffee-script async
	
Now the scripts mentioned below should have everything they need.


Cloud Rendering
---------------

It's very CPU-intensive to render animations with `hqz`, so we include scripts to run a cluster of rendering nodes on the cloud, using Amazon EC2.

* **Warning:** This is all really experimental, and incorrect configurations could result in a large bill from AWS at the end of the month. Use these scripts at your own risk, and only if you know EC2, S3, and SQS well enough to fix things when they break :)

### Animation Format

Animations are represented as JSON files with an array of scene objects, one per frame. Each scene object is processed independently by a different instance of `hqz`.

There is an example animation `examples/branches.coffee`. This is a script which programmatically animates a scene, and writes the resulting JSON array object to stdout.

### Environment

These scripts rely on a handful of environment variables:

* `AWS_ACCESS_KEY_ID` – Your AWS access key. Must have permissions for EC2, S3, and SQS.
* `AWS_SECRET_ACCESS_KEY` – The secret corresponding with your AWS access key. 
* `AWS_REGION` – AWS service region. Go where compute is cheapest if you can. (us-east-1)
* `HQZ_BUCKET` – S3 bucket to use for storage in `queue-submit`. Must exist and be owned by you.

### Work Queue

The included scripts use Amazon's Simple Queue Service to distribute workloads to huge numbers of unreliable rendering nodes. The `queue-runner.coffee` script runs on each rendering node. It retrieves work items from SQS, downloads scene data from S3, renders the scene, uploads the image file to S3, then finally dequeues the work item and sends a completion notification. If the render nodes crash or are disconnected during rendering, the work item will time out and another node will get a chance to collect it.

The `queue-submit.coffee` script submits a new JSON frame array to the cluster. It uploads the scene data and posts work items for each frame. `queue-watcher.coffee` downloads status and completion messages from the cluster, storing them locally in `queue-watcher.log` as well as decoding them to the console as they become available. If you kill and restart `queue-watcher` it will pick up where it left off by replaying `queue-watcher.log` on startup.

The `queue-*` scripts can all be used with or without an EC2 cluster. If you have many idle computers available to you, you can run a `queue-runner` on each, and use AWS only for SQS and S3. This will be very cheap.

### Compute Cluster

If you don't have your own computing cluster, you can rent one inexpensively from Amazon using Spot Instances. This is a way to bid on surplus datacenter capacity, getting prices that are a fraction of the standard price on the condition that your virtual machines can be terminated at any time without notice. As of May 2013, Spot prices for the 8-core virtual machines we use are about 7 cents per hour. This scales linearly. You can rent 20 of these VMs for $1.40/hour, and render 160 frames in parallel.

* **Warning:** You pay for the time your instances are powered on, not just the time they spend rendering frames.

The command `cluster-start.coffee <N>` spawns N cluster nodes. Each one will render up to 8 frames simultaneously. Larger clusters will finish your animation faster, but watch out for unused capacity near the end of the job. You can add and remove nodes during a job without any problem. Start additional nodes by running `cluster-start` again, or stop **all** nodes by running `cluster-stop.coffee`. If you need to stop individual nodes, use the EC2 management console.

Note that `cluster-start` configures the cluster nodes using a shell script supplied via EC2 userdata. The cluster nodes have no permanent storage. On boot, they use `apt-get` and `npm` to install prerequisites, then they download the source for `hqz` from GitHub, compile it, and finally start `queue-runner` in an infinite loop. You can check on the status of these cluster nodes using `cluster-status.coffee`. The optional `-v` flag will also display the system console output from all running nodes.

### Video Encoding

After your render completes, you'll be left with a directory in your S3 bucket full of `.png` files, one for each frame. Unless you have a really fast internet connection, you probably don't want to download all of these files.

* **Warning:** In fact, now seems like a good time to mention that you probably don't want to store all of these files for that long either. S3 charges by the gigabyte-month, and the data transfer fees are higher for moving data out to the internet vs. sending it to EC2. So, the most cost-effective way to deal with all of this data is to compress it on EC2 then delete the originals.

 The `cluster-encode.coffee <job>/<hash>` command creates a fire-and-forget video encoder VM. The command line option is a concatenation of the job name (basename of the file you gave to `cluster-submit`) and a hash of the scene data. You will see this identifier all over the logs from `queue-watcher` as well as in the filenames in your S3 bucket.
 
This command creates a small Spot Instance with a single virtual CPU. Encoding is I/O limited for us, so this VM doesn't need to be especially beefy. It streams the frames directly from S3, compresses a high-quality h264 video suitable as source material for editing or transcoding, then it uploads the resulting video back to S3. It periodically reports progress by uploading a log file to S3. Upon running the script, you'll be given URLs to the log file and to the location where the final video will be stored.


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
    * Number of rays to cast. Larger numbers will take more time to render, but result in smoother images. Lower numbers will be faster, but a "grain" will be visible in the image as you can see the individual rays.

Optional members:

* **"seed"**: *integer*
	* Defines the 32-bit seed value for our pseudorandom number generator. Changing this value will change the specific pattern of noise in the rendering. By default this is arbitrarily set to zero. Controlling the noise pattern may be useful when rendering animations. By default, the PRNG is reinitialized using consecutive seeds for each ray. This means that a stationary pattern of rays will be visible from each light source. By changing the seed, this noise pattern is changed. Another way to think of it: there are a finite number of possible rays that could be traced in any given scene, and we choose to render a range of these rays numbered from `seed` to `seed + rays`. Making small changes to 'seed' will have the effect of cycling new rays in and old rays out. Making large changes in 'seed' will appear to randomize the rays entirely.
* **"gamma"**: *float*
    * Output gamma for the renderer. By default the output is linear, for compatibility with [zenphoton.com](http://zenphoton.com). If this is a nonzero number X, light intensity is raised to the power of 1/x.

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
