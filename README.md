# Leaftest

Leaftest consists of a bash script that uses [minetestmapper](https://github.com/Rogier-5/minetest-mapper-cpp) to display the map on a leaflet based zoomable sloppy map.
For an example, see [VanessaE survival server's map](https://daconcepts.com/vanessa/hobbies/minetest/worldmaps/Survival_World/map.html).

The map is generated through many invocations of `minetestmapper` with small chunks,
instead of one invocation with the whole map, which gets cropped later.
This will keep memory requirements for all tools mostly unrelated to the map size.

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

Make sure that you have provided a `colors.txt` file, e.g. by putting it inside the world directory.

The `dimension` number specifies the width and height of the map that should be rendered, centering with your spawn position. `dimension` should be a multiple of `8*256 = 2048`. `6144` is a good starting value.

The list of parameters passed via the invocation is fixed.
Further options are passed to the mapper script via the
environment variable mechanism (as observable above):

* `MAPPERDIR` must point to the path that contains the minetestmapper executable
* `MAPPERPARAMS` can be used to pass custom parameters to the `minetestmapper` invocation
* `JOBNUM` can be set to an integer > 1 to run `JOBNUM` many processes in parallel to speed up the mapping process

After mapping has finished, you can open the `www/map.html` file. If you want to publish your results, you can either symlink the `www` directory into your `/var/www` directory, or copy it. Due to usage of relative symlinks, you should use `rsync -L`, so that the copied directory doesn't contain symlinks.

### License
Copyright (c) 2015-2016 est31, License: MIT.

Parts base on unilicensed script from [LibertyLand](http://www.ayntest.net/pages/liberty-land-map.html) minetest server, [github here](https://github.com/ayntest/ayntest.github.io).
