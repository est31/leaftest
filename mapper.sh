#!/usr/bin/env bash

MAPDIR=$1
SPAWNPOS=$2
DIMENSIONS=$3

if [ -z $MAPPERDIR ]; then
	MAPPERDIR=.
fi

mapperpath=$MAPPERDIR/minetestmapper

if [ ! -f $mapperpath ]; then
	echo "Error, "$mapperpath" doesn't exist."
	echo "Please specify the path to the minetestmapper executable via the MAPPERDIR variable"
	exit 1
fi

if [ ! -d "$MAPDIR" ]; then
	echo "Error, the directory \"$MAPDIR\" doesn't exist."
	echo "Please specify the path to the minetest world"
	exit 1
fi

if [ -f downscaling/perc ]; then
	echo "Using SSIM based downscaling"
	downscale() {
		downscaling/perc 2 < $1 > ${1}_small
		mv -f ${1}_small $1
	}
else
	echo "Using downscaling provided by imagemagick"
	downscale() {
		convert $1 -resize 50% $1
	}
fi

if [ -z "$JOBNUM" ]; then
	JOBNUM=1
fi

case "$JOBNUM" in
	''|*[!0-9]*) echo "Bad number of jobs '$JOBNUM'. Please specify a positive integer." ; exit 1 ;;
	*) ;;
esac

# echo "Number of jobs: $JOBNUM"

prefix_func="\
prefix() { \
	while read line ; do \
		echo \"\${1}\${line}\"; \
	done \
}"

# retains || usage when doing | prefix "sth"
# use it when needed
prefix_pipefail="\
set -o pipefail ; \
$prefix_func"

# prefix should be usable within this script as well
eval $prefix_func

#sh -c "$prefix_func ; echo -e 'a\nb\nc\n' | prefix 'PREF '"
#bash -c "$prefix_pipefail ; (echo -e 'a\nb\nc\n' ; false) | prefix 'PREF '" || echo "ERR"
#exit 1

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

jobparam="-P$JOBNUM"

echo "Creating tile images with minetestmapper…"
for x in $(seq 0 $tilenum)
do
	for y in $(seq 0 $tilenum)
	do
		posx=$(($spawnx+$tilesize*($x-$tilenum/2)))
		posy=$(($spawny+$tilesize*($tilenum/2-$y)))
		# Execute sh -c "something"
		printf "%s\x00" "-c"
		printf "$prefix_pipefail ; $mapperpath ${MAPPERPARAMS} -i ${MAPDIR} --geometry ${posx},${posy}+${tilesize}+${tilesize} -o ${tiledir}/20/map_${x}_${y}.png \
		2> >(>&2 prefix '[TILEGEN 20/map_${x}_${y}.png ERR]: ') | prefix '[TILEGEN 20/map_${x}_${y}.png]: ' \
		|| (>&2 echo 'minetesmapper for tile [${x},${y}] ended with non zero exit code'; exit 255)\x00"
	done
done | xargs -n2 $jobparam -0 bash # bash required because of "set -o pipefail" usage

xargs_exit=$?
if [ $xargs_exit -ne 0 ]; then
	# xargs unfortunately exits immediately, without even waiting for sub-processes
	sleep 1
	>&2 echo "Error: xargs ended with non-zero exit code $xargs_exit."
	exit 1
fi

# images are joined together and downscaled
echo "Generating lower resolution tile images…"
mult=1
for s in $(seq 1 $zoomlevelnum)
do
	echo "$s/$zoomlevelnum …"
	mult=$(($mult*2))
	tnum=$(($tilenum/$mult-1))
	zoomlevel=$((20-$s))
	zoomlevelbefore=$((20-$s+1))
	zlv=$zoomlevel # shorter name
	dir=$tiledir/$zoomlevel
	dirb=$tiledir/$zoomlevelbefore
	mkdir -p $dir
	rm -f ${dir}/20/*
	for x in $(seq 0 $tnum)
	do
		for y in $(seq 0 $tnum)
		do
			montage $dirb/map_$(($x*2))_$((y*2)).png $dirb/map_$(($x*2+1))_$((y*2)).png $dirb/map_$(($x*2))_$((y*2+1)).png $dirb/map_$(($x*2+1))_$((y*2+1)).png -geometry +0+0 $dir/map_${x}_${y}.png \
			2> >(>&2 prefix "[MONTAGE $zlv/map_${x}_${y}.png ERR]: ") | prefix "[MONTAGE $zlv/map_${x}_${y}.png]: "
			if [ ${PIPESTATUS[0]} -ne 0 ]; then
				echo "montage exited with non zero exit code, aborting."
				exit 1
			fi
			downscale $dir/map_${x}_${y}.png \
			2> >(>&2 prefix "[SHRINK $zlv/map_${x}_${y}.png ERR]: ") | prefix "[SHRINK $zlv/map_${x}_${y}.png]: "
			if [ ${PIPESTATUS[0]} -ne 0 ]; then
				echo "shrinking exited with non zero exit code, aborting."
				exit 1
			fi
		done
	done
done

zoommin=$((20-$zoomlevelnum))
#write the resulting config into a json file
echo "{\"mapsize\":$DIMENSIONS, \"spawn\":{\"x\":$spawnx,\"y\":$spawny}, \"zoommin\":$zoommin}" > $scriptdir/www/conf.json
