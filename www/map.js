var xmlhttp = new XMLHttpRequest();
xmlhttp.onreadystatechange = function() {
    if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
        var config = JSON.parse(xmlhttp.responseText);
        loadmap(config);
    }
}
xmlhttp.open("GET", "conf.json", true);
xmlhttp.overrideMimeType("application/json"); //to silence browser warnings when started without a server
xmlhttp.send();

function loadmap(config) {
	var mapsize = config.mapsize;
	var spawn = config.spawn;
	var zoommin = config.zoommin;
	var xb= 0-spawn.x+mapsize/2;
	var yb= 0-spawn.y-mapsize/2-256;
	var bnd = new L.LatLngBounds();
	bnd.extend(L.latLng([spawn.x-mapsize/2, spawn.y-mapsize/2]));
	bnd.extend(L.latLng([spawn.x+mapsize/2, spawn.y+mapsize/2]));
	var map = L.map('map', {
		maxZoom:26,
		minZoom:zoommin,
		maxNativeZoom:20,
		fullscreenControl: true,
		//maxBounds: bnd, //commented out until it works...
		crs: /*L.CRS.Simple*/L.extend({}, L.CRS, {
			projection: {
				project: function (latlng) {
					return new L.Point(latlng.lat+xb, latlng.lng+yb);
				},

				unproject: function (point) {
					return new L.LatLng(point.x-xb, point.y-yb);
				}
			},
			transformation: new L.Transformation(1, 0, -1, 0),
			scale: function (zoom) {
				return Math.pow(2, zoom-20);
			}
		})
	}).setView([0,0], 22);
	map.setView([spawn.x,spawn.y]);
	L.tileLayer('tiles/{z}/map_{x}_{y}.png', {
		maxZoom: 26,
		maxNativeZoom: 20,
		tileSize: 256,
		continuousWorld: true
	}).addTo(map);
	map.on('mousemove click', function(e) {
		window.mousemove.innerHTML = e.latlng.lat.toFixed(1) + ', ' + e.latlng.lng.toFixed(1);
	});
	var hash = L.hash(map);
}
 
