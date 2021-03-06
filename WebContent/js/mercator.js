function tilex2lon(x, z) {
    return x / Math.pow(2.0, z) * 360.0 - 180;
}

function tiley2lat(y, z) {
    var n = Math.PI - 2.0 * Math.PI * y / Math.pow(2.0, z);
    return 180.0 / Math.PI * Math.atan(0.5 * (Math.exp(n) - Math.exp(-n)));
}

function lon2tilex(lon, z) {
    return ((lon + 180.0) / 360.0 * Math.pow(2.0, z));
}

function lat2tiley(lat, z) {
    return ((1.0 - Math.log(Math.tan(lat * Math.PI / 180.0) + 1.0 / Math.cos(lat * Math.PI / 180.0)) / Math.PI) / 2.0 * Math.pow(2.0, z));
}