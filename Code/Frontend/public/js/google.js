/*
 * google.js
 * This program uses d3.js to create gauges. Gauge appearance is based on similar elements
 * from the google charts API (https://developers.google.com/chart/interactive/docs/gallery/gauge)
 * Programmed by: Rick Menzel (menzelr@oregonstate.edu)
 */
var gauges = [];
var baseGaugeSize = 180;

/*
 * simple helper function to create a gauge with specified params
 */
function createGauge(name, label, min, max, units) {
	var config = {
		size: baseGaugeSize,
		label: label,
		units: units,
		min: undefined != min ? min : 0,
		max: undefined != max ? max : 100,
		minorTicks: 5
	}
	
	var range = config.max - config.min;
	config.yellowZones = [{ from: config.min + range*0.75, to: config.min + range*0.9 }];
	config.redZones = [{ from: config.min + range*0.9, to: config.max }];
					
	gauges[name] = new Gauge(name + "GaugeContainer", config);
	gauges[name].render();
} // end function createGauge()

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

/*
 * simple helper function to invoke gauge creator fn
 */
function createGauges() {
	createGauge("boosterAcceleration",   "BST ACC", 0, 800,  "ft/s²" );	// booster acceleration
	createGauge("boosterVelocity",       "BST VEL", 0, 4500, "ft/s" );	// booster velocity
	//createGauge("pressure",              "PRESSURE");			              // current pressure (active stage)
	//createGauge("thrust",                "THRUST",  0, 2000 );		      // current thrust (active stage)
  createGauge("temperature",           "TEMP",    0, 180, "° F" );		// current temp (active stage)
	createGauge("sustainerVelocity",     "SUS VEL", 0, 4500, "ft/s" );	// sustainer velocity
	createGauge("sustainerAcceleration", "SUS ACC", 0, 800,  "ft/s²" );	// sustainer acceleration
} // end function createGauges()

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

/*
 * devtest function. generates random values to test gauge operation 
 * prior to availability of test data.
 */
function getRandomValue(gauge) {
	var overflow = 0; //10;
	return gauge.config.min - overflow + (gauge.config.max - gauge.config.min + overflow*2) *  Math.random();
} // end function getRandomValue(gauge)
	
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@	
	
/*
 * simple helper function to create and update gauges
 */	
function initGauges() {
	createGauges();
	//setInterval(updateGauges, 5000);
} // end function initGauges()

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

/*
 * primary drawing function for d3.js gauges
 * implementation is intended to replicate the appearance of Google Charts gauges
 * with added functionality to allow for custom appearance
 */
function Gauge(gaugeName, gaugeConfig) {
	this.gaugeName = gaugeName;								                            // get the current gauge name
	
	var self = this;                                                      // for internal d3 functions
	// basic config setup function
	this.configure = function(gaugeConfig) {
		this.config = gaugeConfig;
		// set up gauge position and size
		this.config.size    = this.config.size;// * .9;					            // keep the gauge smaller than the element box (avoids overlap/touching)
		this.config.radius  = this.config.size * .97 * .5;				          // note this is the ENTIRE gauge, including the edge
		this.config.centerX = this.config.size * .5;					              // center the gauge horizontally within the element box
		this.config.centerY = this.config.size * .5;					              // center the gauge vertically within the element box
		// set min/max to gaugeConfig.min/max if defined or static values otherwise.
		this.config.min     = undefined != gaugeConfig.min ? gaugeConfig.min : 0; 	// minimum value of readout (left limit)
		this.config.max     = undefined != gaugeConfig.max ? gaugeConfig.max : 100; // maximum value of readout (right limit)
		this.config.range   = this.config.max - this.config.min;			      // difference between min and max values
		// tick counts gaugeConfig.majorTicks/minorTicks if defined or static values otherwise.
		this.config.majorTicks = gaugeConfig.majorTicks || 5;				        // number of major ticks
		this.config.minorTicks = gaugeConfig.minorTicks || 2;				        // number of minor ticks (regions, actual ticks will be one less as the last will coincide with a major tick)
		// custom color values for subregions regions if desired 
		this.config.normalColor  = gaugeConfig.normalColor  || "#109618";		// normal zone
		this.config.warningColor = gaugeConfig.warningColor || "#FF9900";		// warning zone "#FAFF00";
		this.config.dangerColor  = gaugeConfig.dangerColor  || "#DC3912";		// danger zone! "#FF0000";
		
		this.config.transitionDuration = gaugeConfig.transitionDuration || 500;
	} // end configure function
	
	// graphics display() function 
	this.render = function() {
		// get the page element corresponding to the gauge body and perform setup
		this.body = d3.select("#" + this.gaugeName)
			            .append("svg:svg")			            // this is a scalable vector graphic
			            .attr("class", "gauge")			        // for external css, etc.	
			            .attr("width",  this.config.size)		// SVG element absolute width
			            .attr("height", this.config.size);	// SVG element absolute height
		// configure the outer circle/edge region
		this.body.append("svg:circle")				            // use predefined svg circle shape
			       .attr("cx", this.config.centerX)		      // outer circle center (x-coord)
		         .attr("cy", this.config.centerY)		      // outer circle center (y-coord)
			       .attr("r",  this.config.radius)		      // outer circle radius (full gauge radius)
			       .style("fill", "#7a7a7a")			          // outer circle color (edge region color)
			       .style("stroke", "#000")			            // outer circle edge line color
			       .style("stroke-width", ".5px");		      // outer circle edge line weight
		// configure the inner circle/gauge fill region			
		this.body.append("svg:circle")				            // use predefined svg circle shape
			      .attr("cx", this.config.centerX)		      // inner circle center (x-coord)
			      .attr("cy", this.config.centerY)		      // inner circle center (y-coord)
			      .attr("r", 0.9 * this.config.radius)	    // inner circle radius (fits inside above)
			      .style("fill", "#1e1e1e")			            // inner circle color (gauge fill)
			      .style("stroke", "#303030")		            // inner circle edge line color
			      .style("stroke-width", "2px");		        // inner circle edge line weight
		// draw the greenzone (normal) arc			
		for (var index in this.config.greenZones) {
			this.drawBand(this.config.greenZones[index].from, this.config.greenZones[index].to, self.config.normalColor);
		}
		// draw the yellowzone (warning) arc
		for (var index in this.config.yellowZones) {
			this.drawBand(this.config.yellowZones[index].from, this.config.yellowZones[index].to, self.config.warningColor);
		}
		// draw the redzone (danger) arc
		for (var index in this.config.redZones) {
			this.drawBand(this.config.redZones[index].from, this.config.redZones[index].to, self.config.dangerColor);
		}
		// handle gauges with negative values
		if (this.config.min < 0) {
			// draw the yellowzone (warning) arc in the negative zone
			for (var index in this.config.yellowZones) {
				this.drawBand(-1. * this.config.yellowZones[index].to, -1. * this.config.yellowZones[index].from, self.config.warningColor);
			}
			// draw the redzone (danger) arc in the negative zone
			for (var index in this.config.redZones) {
				this.drawBand(-1. * this.config.redZones[index].to, -1. * this.config.redZones[index].from, self.config.dangerColor);
			}
		}
		// configure main gauge (data type) labels
		if (undefined != this.config.label) {
			var fontSize = Math.round(this.config.size / 9.);	    // automatically size to fit depending on total element size
			this.body.append("svg:text")				                  // use predefined svg text config
				 .attr("x", this.config.centerX)		                // center the label horizontally
				 .attr("y", this.config.centerY / 2 + fontSize / 2) // offset the label from center vertically to avoid overlap with hub
				 .attr("dy", fontSize / 2)			                    // offset distance		
				 .attr("text-anchor", "middle")			                // center the text (css)
				 .text(this.config.label)                           // populate label text
				 .style("font-size", fontSize + "px")		            // set font size (css)
				 .style("fill", "#fff")				                      // set label color
				 .style("stroke-width", "0px");			                // set text border thickness
		}
		// configure unit labels; IMPORTANT: html MUST have 'content="text/html;charset=utf-8"' attribute set or squared signs in units will not display correctly
		if (undefined != this.config.units) {
			var fontSize = Math.round(this.config.size / 12.);	  // automatically size to fit depending on total element size
			this.body.append("svg:text")				                  // use predefined svg text config
				 .attr("x", this.config.centerX)		                // center the label horizontally
				 .attr("y", this.config.centerY + fontSize * 1.5)  	// offset the label from center vertically to avoid overlap with hub
				 .attr("dy", fontSize / 2)			                    // offset distance		
				 .attr("text-anchor", "middle")			                // center the text (css)
				 .text(this.config.units)                           // populate label text
				 .style("font-size", fontSize + "px")		            // set font size (css)
				 .style("fill", "#fff")				                      // set label color (white)
				 .style("stroke-width", "0px");			                // set text border thickness
		}
		
		// configure min/max labels and tick marks
		var fontSize = Math.round(this.config.size / 16);		    // automatically size to fit depending on total element size
		var majorDelta = this.config.range / (this.config.majorTicks - 1);	// find out how far between major ticks
		// walk across the data range in steps equal to distance between major ticks
		for (var major = this.config.min; major <= this.config.max; major += majorDelta) {
			
			var minorDelta = majorDelta / this.config.minorTicks;	// how far between minor ticks
			// further subdivide region between major ticks and walk across the data range in steps equal to distance between minor ticks
			for (var minor = major + minorDelta; minor < Math.min(major + majorDelta, this.config.max); minor += minorDelta) {
				// configure minor ticks
				var point1 = this.valueToPoint(minor, 0.75);	      // how far toward center does minor tick extend (inner threshold , higher number = higher radius)
				var point2 = this.valueToPoint(minor, 0.85);	      // how far away from center does minor tick extend (outer threshold, higher number = higher radius)
				this.body.append("svg:line")			                  // use predefined svg line config
					 .attr("x1", point1.x)			                      // minor tick x position start point
					 .attr("y1", point1.y)			                      // minor tick y position start point
					 .attr("x2", point2.x)			                      // minor tick x position end point
					 .attr("y2", point2.y)			                      // minor tick y position end point
					 //.style("stroke", "#ff7b00")		                // minor tick color
					 .style("stroke", "#fff")		                      // minor tick color
					 .style("stroke-width", "1px");		                // minor tick weight
			}
			// configure major ticks
			var point1 = this.valueToPoint(major, 0.7);		        // how far toward center does major tick extend (inner threshold , higher number = higher radius)
			var point2 = this.valueToPoint(major, 0.85);		      // how far away from center does major tick extend (outer threshold, higher number = higher radius)
			this.body.append("svg:line")				                  // use predefined svg line config		
				 .attr("x1", point1.x)				                      // major tick x position start point
				 .attr("y1", point1.y)				                      // major tick y position start point
				 .attr("x2", point2.x)				                      // major tick x position end point
				 .attr("y2", point2.y)				                      // major tick y position end point
				 .style("stroke", "#7a7a7a")			                  // major tick color
				 .style("stroke-width", "3px");			                // major tick weight
			// if this major tick is the max, min or midpoint, we want to label it
			if (major == this.config.min || major == this.config.max || major == (this.config.max / 2) ) {
				var point = this.valueToPoint(major, 0.59);	        // position the label (how far from center, higher number = higher radius)
				this.body.append("svg:text")			                  // use predefined svg text config
				 	 .attr("x", point.x)			                        // position x coord
				 	 .attr("y", point.y)			                        // position y coord
				 	 .attr("dy", fontSize / 3)		                    // min/max label vertical offset (depends on text size)
				 	 .attr("text-anchor", major == this.config.min ? "start" : major == this.config.max ? "end" : "middle") // if min, is "start"; if max, is "end"; else is middle
				 	 .text(major)				                              // text element corresponding to major tick
				 	 .style("font-size", fontSize + "px")	            // min/max label text size
					 .style("fill", "#ffffff")		                    // min/max label text color 
					 .style("stroke-width", "0px");		                // min/max label edge weight
			}
		}
		// get the page element corresponding to the gauge needle and perform setup
		var pointerContainer = this.body.append("svg:g").attr("class", "pointerContainer");
		// find the mid point of this gauge
		var midValue = (this.config.min + this.config.max) / 2;
		// start the gauge at center - NOTE: this should change for each gauge, potentially
		var pointerPath = this.buildPointerPath(midValue);
		// the needle is positioned between the last and next value with interpolation depending on time
		var pointerLine = d3.svg.line()
                        .x(function(d) { return d.x })
				                .y(function(d) { return d.y })
				                .interpolate("basis");
		// configure the needle
		pointerContainer.selectAll("path")
				.data([pointerPath])			            // needle positioning
				.enter()				                      //  
				.append("svg:path")			              // use svg path class
				.attr("d", pointerLine)			          //
				.style("fill", "#ff7b00")		          // needle fill color
				.style("stroke", "#c63310")		        // needle edge color
				.style("stroke-width", "1px")         // needle edge width
				.style("fill-opacity", 0.7);		      // needle transparency
		// configure the central hub			
		pointerContainer.append("svg:circle")			// use predefined svg circle shape
				.attr("cx", this.config.centerX)	    // hub center (x-coord)
				.attr("cy", this.config.centerY)	    // hub center (y-coord)
				.attr("r", 0.12 * this.config.radius)	// hub radius
				.style("fill", "#383838")		          // hub color
				.style("stroke", "#000")		          // hub edge color
				.style("opacity", 1);			            // hub transparency
		// configure numerical readout (data value) labels
		var fontSize = Math.round(this.config.size / 10);	// automatically size to fit depending on total element size
		pointerContainer.selectAll("text")			  // select the text value from the pointer container (in other words this is the value corresponding to the pointer's location)
				.data([midValue])
				.enter()
				.append("svg:text")			              // use predefined svg text config
				.attr("x", this.config.centerX)		    // center the label horizontally
				.attr("y", this.config.size - this.config.centerY / 4 - fontSize * .5) // offset the label from center vertically to avoid overlap with hub
				.attr("dy", fontSize / 2)		          // offset distance
				.attr("text-anchor", "middle")		    // center the text (css)
				.style("font-size", fontSize + "px")	// set font size (css)
				.style("fill", "#ff7b00")		          // set label color
				.style("stroke-width", "0px");		    // set text border thickness
		// 
		this.redraw(this.config.min, 0);			    // set gauge default to 0 on page load
	} // end render function
	
	// figure out where the needle goes
	this.buildPointerPath = function(value)	{
		var delta = this.config.range / 13;
		
		var head0 = valueToPoint(value, 0.85);
		var head1 = valueToPoint(value - delta, 0.12);
		var head2 = valueToPoint(value + delta, 0.12);
		
		var tailValue = value - (this.config.range * (1/(270/360)) / 2);
		var tail0 = valueToPoint(tailValue, 0.28);
		var tail1 = valueToPoint(tailValue - delta, 0.12);
		var tail2 = valueToPoint(tailValue + delta, 0.12);
		
		return [head0, head1, tail2, tail0, tail1, head2, head0];
		
		function valueToPoint(value, factor) {
			var point = self.valueToPoint(value, factor);
			point.x -= self.config.centerX;
			point.y -= self.config.centerY;
			return point;
		}
	} // end buildPointerPath function
	
	// function for configuring normal/warning/danger zone arcs
	// need to add logic for gauges with negative values
	this.drawBand = function(start, end, color) {
		if (0 >= end - start) {							              // there cannot be a negatively sized arc (this would mess with the others)
			return;						
		}
		this.body.append("svg:path")						
			 .style("fill", color)						              // color the arc appropriately		
			 .attr("d", d3.svg.arc()					              // use d3's built-in arc element
					  // determine arc size (run)
					  .startAngle(this.valueToRadians(start))	  // get the arc start point
			 		  .endAngle(this.valueToRadians(end))		    // get the arc end point
					  // determine arc width (thickness relative to radius)
			 		  .innerRadius(0.65 * this.config.radius)	  // arc outer termination 
			 		  .outerRadius(0.85 * this.config.radius))	// arc outer termination 
			 // translate/rotate the arc to place it where we want it (accounts for the gap on the bottom and other gauge orientation, starting position is at 12 o'clock)
			 .attr("transform", function() { 
				return "translate(" + self.config.centerX + ", " + self.config.centerY + ") rotate(270)"; 
			 });
	} // end drawBand function
	
	// do animation between prev and curr values (essentially the Animate() function)
	this.redraw = function(value, transitionDuration) {
		var pointerContainer = this.body.select(".pointerContainer");			// only thing being animated is the pointer
		pointerContainer.selectAll("text").text(Math.round(value));				// get the readout (in the pointer container) and round it to integer
		
		var pointer = pointerContainer.selectAll("path");
		pointer.transition()
		       .duration(undefined != transitionDuration ? transitionDuration : this.config.transitionDuration)
		       //.delay(0)
		       //.ease("linear")
		       //.attr("transform", function(d) 
		       .attrTween("transform", function() {
		       		var pointerValue = value;						    // determine the current pointer (readout) value
		       		// do "pegging" beyond limits
				if (value > self.config.max) {						    // if beyond max, peg the pointer
					pointerValue = self.config.max + 0.02*self.config.range;
				} else if (value < self.config.min) {					// if beyond min, peg the pointer
					pointerValue = self.config.min - 0.02*self.config.range;
				}
				var targetRotation = (self.valueToDegrees(pointerValue) - 90);
				var currentRotation = self._currentRotation || targetRotation;
				self._currentRotation = targetRotation;
				
				return function(step) {
					var rotation = currentRotation + (targetRotation-currentRotation)*step;
					return "translate(" + self.config.centerX + ", " + self.config.centerY + ") rotate(" + rotation + ")"; 
				}
	               });
	} // end redraw function
	
	// convert value (location along min/max arc) to degrees for mapping
	this.valueToDegrees = function(value) {
		// 270 degrees accounts for starting position at 12 o'clock and bottom gap
		return value / this.config.range * 270 - (this.config.min / this.config.range * 270 + 45);
	} // end valueToDegrees function
	
	// convert value (location along min/max arc) to radians
	this.valueToRadians = function(value) {
		return this.valueToDegrees(value) * Math.PI / 180;
	} // end valueToRadians function
	
	// convert value (location along min/max arc) to a point; NOTE: returns xy tuple
	this.valueToPoint = function(value, factor) {
		return { 	
			x: this.config.centerX - this.config.radius * factor * Math.cos(this.valueToRadians(value)),
			y: this.config.centerY - this.config.radius * factor * Math.sin(this.valueToRadians(value)) 		
		};
	} // end valueToPoint function
	
	// initialization
	this.configure(gaugeConfig);	
} // end def function Gauge(gaugeName, gaugeConfig)