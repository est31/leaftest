#!/bin/bash

MAPDIR=$1
SPAWNPOS=$2
DIMENSIONS=$3

echo "map dir set to "$MAPDIR

if [ -z $MAPPERDIR ]; then
	MAPPERDIR=.
fi

MAPPERDIR=$MAPPERDIR/minetestmapper

if [ ! -e  $MAPPERDIR ]; then
	echo "error, "$MAPPERDIR" doesn't exist."
	exit
fi

scriptdir=`readlink -f $0`
scriptdir=`dirname $scriptdir`

tiledir=$scriptdir/www/tiles

spawnx=${SPAWNPOS%%,*}
spawny=${SPAWNPOS##*,}
#echo "spawn position: $spawnx $spawny"
tilesize=256
zoomlevelnum=3

tilenum=$(($DIMENSIONS/$tilesize))

mkdir -p ${tiledir}/20
rm -f ${tiledir}/20/*

#create tile images
for x in $(seq 0 $tilenum)
do
	for y in $(seq 0 $tilenum)
	do
		posx=$(($spawnx+$tilesize*($x-$tilenum/2)))
		posy=$(($spawny+$tilesize*($tilenum/2-$y)))
		$MAPPERDIR ${MAPPERPARAMS} -i ${MAPDIR} --geometry ${posx}:${posy}+${tilesize}+${tilesize} -o ${tiledir}/20/map_${x}_${y}.png &
	done &
done

delay=0
while [ $(ps -C minetestmapper | wc -l) != 1 ]
do
	#minetestmappers are unfinished
	sleep 1
	delay=$(($delay+1))
	echo $delay" seconds slept waiting for minetestmapper"
done

#join the images and make them smaller
mult=1
for s in $(seq 1 $zoomlevelnum)
do
	mult=$(($mult*2))
	tnum=$(($tilenum/$mult-1))
	zoomlevel=$((20-$s))
	zoomlevelbefore=$((20-$s+1))
	dir=$tiledir/$zoomlevel
	dirb=$tiledir/$zoomlevelbefore
	mkdir -p $dir
	rm -f ${dir}/20/*
	for x in $(seq 0 $tnum)
	do
		for y in $(seq 0 $tnum)
		do
			montage $dirb/map_$(($x*2))_$((y*2)).png $dirb/map_$(($x*2+1))_$((y*2)).png $dirb/map_$(($x*2))_$((y*2+1)).png $dirb/map_$(($x*2+1))_$((y*2+1)).png -geometry +0+0 $dir/map_${x}_${y}.png
			convert $dir/map_${x}_${y}.png -resize 50% $dir/map_${x}_${y}.png
		done
	done
done

zoommin=$((20-$zoomlevelnum))
#write the resulting config into a json file
echo "{\"mapsize\":$DIMENSIONS, \"spawn\":{\"x\":$spawnx,\"y\":$spawny}, \"zoommin\":$zoommin}" > $scriptdir/www/conf.json
