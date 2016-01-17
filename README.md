#Leaftest

Leaftest consists of a bash script that uses [minetestmapper](https://github.com/Rogier-5/minetest-mapper-cpp) to display the map on a leaflet based zoomable sloppy map. As an example, see [VanessaE survival server's map](https://daconcepts.com/vanessa/hobbies/minetest/worldmaps/Survival_World/map.html).

The map is generated in chunks through many invocations of `minetestmapper` instead of one invocation and large file thats cropped later, so that memory requirements for all tools are unrelated to the map size.

### Dependencies

Ensure that you have:

    -bash
    -imagemagick
    -minetestmapper

If you want to host the map on the world wide web, you'll need a web server.

### Running the mapper script

Get a clone:
```
git clone --recursive https://github.com/est31/leaftest.git
```

To start the mapping, do:
```
MAPPERDIR=dir/containing/minetestmapper ./mapper.sh path/to/world spawnx,spawny dimension
```

The `dimension` number specifies the width and height of the map that should be rendered, centering with your spawn position. `dimension` should be a multiple of 8*256 =  2048, 6144 is a good starting value.

The list of parameters passed via the invocation is fixed,
but you can use the environment variable mechanism
to pass further options to the mapper script:

* `MAPPERDIR` must point to the path that contains the minetestmapper executable
* `MAPPERPARAMS` can be used to pass custom parameters to the `minetestmapper` invocation
* `JOBNUM` can be used to enable parallelisation. Set it to a positive number to

After mapping has finished, you can open the `www/map.html` file. If you want to publish your results, you can either symlink the `www` directory into your `/var/www` directory, or copy it. Due to usage of relative symlinks, you should use `rsync -L`, so that the copied directory doesn't contain symlinks.

### License
Copyright (c) 2015-2016 est31, License: MIT.

Parts base on unilicensed script from [LibertyLand](http://www.ayntest.net/pages/liberty-land-map.html) minetest server, [github here](https://github.com/ayntest/ayntest.github.io).
