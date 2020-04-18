var backgroundMapStyle = function(tilesEndpoint) {
  const water_overview = "hsl(0, 0%, 70%)";
    const water = "hsl(0, 0%, 70%)";
    
    const rail_overvew = "hsl(0, 0%, 65%)";
    const rail = "hsl(0, 0%, 50%)";
    
    const pedestrian = "hsl(0, 0%, 90%)";
    
    const sport = "hsla(0, 0%, 70%, 35%)";
    const sport_outline = "hsla(0, 0%, 70%, 45%)";
    
    const building = "hsla(0, 0%, 70%, 35%)";
    const building_outline = "hsla(0, 0%, 70%, 45%)";
    
    const roadLineWidth = (add) => {
      return [
        "let",
        "base", ["match", ["get", "highway"],
          "motorway", 3,
          ["trunk", "motorway_link"], 2.75,
          ["primary", "trunk_link"], 2.5,
          ["secondary", "aeroway"], 2.25,
          ["primary_link", "secondary_link", "tertiary", "tertiary_link"], 1.75,
          ["residential", "unclassified"], 1.5,
        0.75],
        ["interpolate", ["linear"], ["zoom"],
          5,  ["+", ["*", ["var", "base"], 0.1], add],
          9,  ["+", ["*", ["var", "base"], 0.4], add],
          12, ["+", ["*", ["var", "base"], 1 * 1.25], add],
          16, ["+", ["*", ["var", "base"], 4 * 1.5], add],
          20, ["+", ["*", ["var", "base"], 8 * 1.5], add]
        ]
      ];
    };

    return {
      "version": 8,
      "sources": {
          "osm": {
              "type": "vector",
              "tiles": [`${tilesEndpoint}tiles/{z}/{x}/{y}.mvt`],
              "maxzoom": 20
          }
      },
      "glyphs": `${tilesEndpoint}tiles/glyphs/{fontstack}/{range}.pbf`,
      "layers": [
          {
              "id": "background",
              "type": "background",
              "paint": { "background-color": "#dddddd" }
          }, {
              "id": "coastline",
              "type": "fill",
              "source": "osm",
              "source-layer": "coastline",
              "paint": {
                "fill-color": ["interpolate-hcl", ["linear"], ["zoom"],
                                6, water_overview, 9, water],
              }
          }, {
              "id": "landuse",
              "type": "fill",
              "source": "osm",
              "source-layer": "landuse",
              "paint": {
                  "fill-color": ["match", ["get", "landuse"],
                      "complex", "hsl(0, 0%, 79%)",
                      "commercial", "hsl(0, 0%, 82%)",
                      "industrial", "hsl(0, 0%, 82%)",
                      "residential", "hsl(0, 0%, 85%)",
                      "retail", "hsl(0, 0%, 81%)",
                      "construction", "magenta",
    
                      "nature_light", "hsl(0, 0%, 91%)",
                      "nature_heavy", "hsl(0, 0%, 88%)",
                      "park", "hsl(0, 0%, 90%)",
                      "cemetery", "hsl(0, 0%, 90%)",
                      "beach", "hsl(0, 0%, 90%)",
    
                      "#ff0000"]
              }
          }, {
              "id": "water",
              "type": "fill",
              "source": "osm",
              "source-layer": "water",
              "paint": {
                "fill-color": ["interpolate-hcl", ["linear"], ["zoom"],
                                6, water_overview, 9, water],
              }     
          }, {
              "id": "sport",
              "type": "fill",
              "source": "osm",
              "source-layer": "sport",
              "paint": {
                  "fill-color": sport,
                  "fill-outline-color": sport_outline
              }
          }, {
              "id": "pedestrian",
              "type": "fill",
              "source": "osm",
              "source-layer": "pedestrian",
              "paint": {"fill-color": pedestrian}
          }, {
              "id": "waterway",
              "type": "line",
              "source": "osm",
              "source-layer": "waterway",
              "paint": {
                "line-color": ["interpolate-hcl", ["linear"], ["zoom"],
                                6, water_overview, 9, water],
              }
          }, {
              "id": "road_back",
              "type": "line",
              "source": "osm",
              "source-layer": "road",
              "filter": [
                "!in",
                "highway",
                "footway", "track", "steps", "cycleway", "path",
              ],
              "layout": {
                "line-cap": "round",
              },
              "paint": {
                "line-color": "hsl(0, 0%, 70%)",
                "line-opacity": 0.5,
                "line-width": roadLineWidth(1)
              }
          }, {
              "id": "road",
              "type": "line",
              "source": "osm",
              "source-layer": "road",
              "layout": {
                "line-cap": "round",
              },
              "paint": {
                "line-color": "hsl(0, 0%, 98%)",
                "line-opacity": ["match", ["get", "highway"],
                  ["footway", "track", "steps", "cycleway", "path"], 0.66,
                  1],
                  "line-width": roadLineWidth(0)
              }
          }, {
              "id": "rail_old",
              "type": "line",
              "source": "osm",
              "source-layer": "rail",
              "filter": ["==", "rail", "old"],
              "layout": { "line-cap": "round", },
              "paint": { "line-color": rail, }
          }, {
              "id": "rail_detail",
              "type": "line",
              "source": "osm",
              "source-layer": "rail",
              "filter": ["==", "rail", "detail"],
              "layout": { "line-cap": "round", },
              "paint": { "line-color": rail, }
          }, {
              "id": "rail_secondary",
              "type": "line",
              "source": "osm",
              "source-layer": "rail",
              "filter": ["==", "rail", "secondary"],
              "layout": { "line-cap": "round", },
              "paint": {
                  "line-color": rail,
                  "line-width": 1.15
              }
          }, {
              "id": "rail_primary",
              "type": "line",
              "source": "osm",
              "source-layer": "rail",
              "filter": ["==", "rail", "primary"],
              "layout": { "line-cap": "round", },
              "paint": {
                  "line-color": ["interpolate-hcl", ["linear"], ["zoom"],
                                  6, rail_overvew, 9, rail],
                  "line-width": ["interpolate", ["linear"], ["zoom"],
                  6, 1, 9, 1.3],
              }
          }, {
              "id": "building",
              "type": "fill",
              "source": "osm",
              "source-layer": "building",
              "paint": {
                  "fill-color": building,
                  "fill-outline-color": building_outline
              }
          }, {
              "id": "road-ref-shield",
              "type": "symbol",
              "source": "osm",
              "source-layer": "road",
              "minzoom": 6,
              "filter": ["any", ["==", ["get", "highway"], "motorway"],
                                ["==", ["get", "highway"], "trunk"],
                                ["==", ["get", "highway"], "secondary"],
                                [">", ["zoom"], 11]],
              "layout": {
                "symbol-placement": "line",
                "text-field": ["get", "ref"],
                "text-font": ["Noto Sans Display Regular"],
                "text-size": 10,
                "text-rotation-alignment": "viewport"
              },
              "paint": {
                "text-halo-width": 10,
                "text-halo-color": "white",
                "text-color": "#333333"
              }
          }, {
              "id": "road-name-text",
              "type": "symbol",
              "source": "osm",
              "source-layer": "road",
              "minzoom": 14,
              "layout": {
                "symbol-placement": "line",
                "text-field": ["get", "name"],
                "text-font": ["Noto Sans Display Regular"],
                "text-size": 10,
              },
              "paint": {
                "text-halo-width": 11,
                "text-halo-color": "white",
                "text-color": "#333333"
              }
          }, {
              "id": "towns",
              "type": "symbol",
              "source": "osm",
              "source-layer": "cities",
              "filter": ["all", 
                  ["!=", ["get", "place"], "city"],
                  ["any", [">=", ["zoom"], 11], 
                          [">", ["coalesce", ["get", "population"], 0], 10000]]
                ],
              "layout": {
                "symbol-sort-key": ["-", ["coalesce", ["get", "population"], 0]],
                "text-field": ["get", "name"],
                "text-font": ["Noto Sans Display Regular"],
                "text-size": 12
              },
              "paint": {
                "text-halo-width": 1,
                "text-halo-color": "white",
                "text-color": "hsl(0, 0%, 30%)"
              }
          }, {
              "id": "cities",
              "type": "symbol",
              "source": "osm",
              "source-layer": "cities",
              "filter": ["==", ["get", "place"], "city"],
              "layout": {
                "symbol-sort-key": ["-", ["coalesce", ["get", "population"], 0]],
                "text-field": ["get", "name"],
                "text-font": ["Noto Sans Display Bold"],
                "text-size": ["interpolate", ["linear"], ["zoom"],
                              6, 12, 
                              9, 16],
              },
              "paint": {
                "text-halo-width": 2,
                "text-halo-color": "white",
                "text-color": "hsl(0, 0%, 20%)"
              }
          }
      ]
    };
}
