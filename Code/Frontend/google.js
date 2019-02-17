function Gauge(placeholderName, configuration) {
	this.placeholderName = placeholderName;
	
	var self = this; // for internal d3 functions
	// basic config setup function
	this.configure = function(configuration) {
		this.config = configuration;
		// set up gauge position and size
		this.config.size = this.config.size * .9;					// keep the gauge smaller than the element box (avoids overlap/touching)
		this.config.radius = this.config.size * .97 * .5;				// note this is the ENTIRE gauge, including the edge
		this.config.cx = this.config.size * .5;						// center the gauge horizontally within the element box
		this.config.cy = this.config.size * .5;						// center the gauge vertically within the element box
		// set min/max to configuration.min/max if defined or static values otherwise.
		this.config.min = undefined != configuration.min ? configuration.min : 0; 	// minimum value of readout (left limit)
		this.config.max = undefined != configuration.max ? configuration.max : 100; 	// maximum value of readout (right limit)
		this.config.range = this.config.max - this.config.min;				// difference between min and max values
		// tick counts configuration.majorTicks/minorTicks if defined or static values otherwise.
		this.config.majorTicks = configuration.majorTicks || 5;				// number of major ticks
		this.config.minorTicks = configuration.minorTicks || 2;				// number of minor ticks (regions, actual ticks will be one less as the last will coincide with a major tick)
		// custom color values for subregions regions if desired 
		this.config.normalColor  = configuration.normalColor  || "#109618";		// normal zone
		this.config.warningColor = configuration.warningColor || "#FF9900";		// warning zone
		this.config.dangerColor  = configuration.dangerColor  || "#DC3912";		// danger zone!
		
		this.config.transitionDuration = configuration.transitionDuration || 500;
	} // end configure function
	
	// graphics display() function 
	this.render = function() {
		// get the page element corresponding to the gauge body and perform setup
		this.body = d3.select("#" + this.placeholderName)
			      .append("svg:svg")			// this is a scalable vector graphic
			      .attr("class", "gauge")
			      .attr("width", this.config.size)		// SVG element absolute width
			      .attr("height", this.config.size);	// SVG element absolute height
		// configure the outer circle/edge region
		this.body.append("svg:circle")				// use predefined svg circle shape
			      .attr("cx", this.config.cx)		// outer circle center (x-coord)
		              .attr("cy", this.config.cy)		// outer circle center (y-coord)
			      .attr("r", this.config.radius)		// outer circle radius (full gauge radius)
			      .style("fill", "#ccc")			// outer circle color (edge region color)
			      .style("stroke", "#000")			// outer circle edge line color
			      .style("stroke-width", ".5px");		// outer circle edge line weight
		// configure the inner circle/gauge fill region			
		this.body.append("svg:circle")				// use predefined svg circle shape
			      .attr("cx", this.config.cx)		// inner circle center (x-coord)
			      .attr("cy", this.config.cy)		// inner circle center (y-coord)
			      .attr("r", 0.9 * this.config.radius)	// inner circle radius (fits inside above)
			      .style("fill", "#fff")			// inner circle color (gauge fill)
			      .style("stroke", "#e0e0e0")		// inner circle edge line color
			      .style("stroke-width", "2px");		// inner circle edge line weight
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
		// configure main gauge (data type) labels
		if (undefined != this.config.label) {
			var fontSize = Math.round(this.config.size / 9.);	// automatically size to fit depending on total element size
			this.body.append("svg:text")				// use predefined svg text config
				 .attr("x", this.config.cx)			// center the label horizontally
				 .attr("y", this.config.cy / 2 + fontSize / 2)  // offset the label from center vertically to avoid overlap with hub
				 .attr("dy", fontSize / 2)			// offset distance		
				 .attr("text-anchor", "middle")			// center the text (css)
				 .text(this.config.label)
				 .style("font-size", fontSize + "px")		// set font size (css)
				 .style("fill", "#333")				// set label color
				 .style("stroke-width", "0px");			// set text border thickness
		}
		// configure min/max labels and tick marks
		var fontSize = Math.round(this.config.size / 16);		// automatically size to fit depending on total element size
		var majorDelta = this.config.range / (this.config.majorTicks - 1);	// find out how far between major ticks
		// walk across the data range in steps equal to distance between major ticks
		for (var major = this.config.min; major <= this.config.max; major += majorDelta) {
			
			var minorDelta = majorDelta / this.config.minorTicks;	// how far between minor ticks
			// further subdivide region between major ticks and walk across the data range in steps equal to distance between minor ticks
			for (var minor = major + minorDelta; minor < Math.min(major + majorDelta, this.config.max); minor += minorDelta) {
				// configure minor ticks
				var point1 = this.valueToPoint(minor, 0.75);	// how far toward center does minor tick extend (inner threshold , higher number = higher radius)
				var point2 = this.valueToPoint(minor, 0.85);	// how far away from center does minor tick extend (outer threshold, higher number = higher radius)
				this.body.append("svg:line")			// use predefined svg line config
					 .attr("x1", point1.x)			// minor tick x position start point
					 .attr("y1", point1.y)			// minor tick y position start point
					 .attr("x2", point2.x)			// minor tick x position end point
					 .attr("y2", point2.y)			// minor tick y position end point
					 .style("stroke", "#666")		// minor tick color
					 .style("stroke-width", "1px");		// minor tick weight
			}
			// configure major ticks
			var point1 = this.valueToPoint(major, 0.7);		// how far toward center does major tick extend (inner threshold , higher number = higher radius)
			var point2 = this.valueToPoint(major, 0.85);		// how far away from center does major tick extend (outer threshold, higher number = higher radius)
			this.body.append("svg:line")				// use predefined svg line config		
				 .attr("x1", point1.x)				// major tick x position start point
				 .attr("y1", point1.y)				// major tick y position start point
				 .attr("x2", point2.x)				// major tick x position end point
				 .attr("y2", point2.y)				// major tick y position end point
				 .style("stroke", "#333")			// major tick color
				 .style("stroke-width", "2px");			// major tick weight
			// if this major tick is the max or min, we want to label it
			if (major == this.config.min || major == this.config.max)
			{
				var point = this.valueToPoint(major, 0.63);	// position the label (how far from center, higher number = higher radius)
				this.body.append("svg:text")			// use predefined svg text config
				 	 .attr("x", point.x)			// position x coord
				 	 .attr("y", point.y)			// position y coord
				 	 .attr("dy", fontSize / 3)		// min/max label vertical offset (depends on text size)
				 	 .attr("text-anchor", major == this.config.min ? "start" : "end")
				 	 .text(major)				// text element corresponding to major tick
				 	 .style("font-size", fontSize + "px")	// min/max label text size
					 .style("fill", "#333")			// min/max label text color 
					 .style("stroke-width", "0px");		// min/max label edge weight
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
				.data([pointerPath])			// needle positioning
				.enter()				
				.append("svg:path")			
				.attr("d", pointerLine)			
				.style("fill", "#dc3912")		// needle fill color
				.style("stroke", "#c63310")		// needle edge color
				.style("fill-opacity", 0.7)		// needle transparency
		// configure the central hub			
		pointerContainer.append("svg:circle")			// use predefined svg circle shape
				.attr("cx", this.config.cx)		// hub center (x-coord)
				.attr("cy", this.config.cy)		// hub center (y-coord)
				.attr("r", 0.12 * this.config.radius)	// hub radius
				.style("fill", "#4684EE")		// hub color
				.style("stroke", "#666")		// hub edge color
				.style("opacity", 1);			// hub transparency
		// configure numerical readout (data value) labels
		var fontSize = Math.round(this.config.size / 10);	// automatically size to fit depending on total element size
		pointerContainer.selectAll("text")			// select the text value from the pointer container (in other words this is the value corresponding to the pointer's location)
				.data([midValue])
				.enter()
				.append("svg:text")			// use predefined svg text config
				.attr("x", this.config.cx)		// center the label horizontally
				.attr("y", this.config.size - this.config.cy / 4 - fontSize) // offset the label from center vertically to avoid overlap with hub
				.attr("dy", fontSize / 2)		// offset distance
				.attr("text-anchor", "middle")		// center the text (css)
				.style("font-size", fontSize + "px")	// set font size (css)
				.style("fill", "#000")			// set label color
				.style("stroke-width", "0px");		// set text border thickness
		// 
		this.redraw(this.config.min, 0);			// set gauge default to 0 on page load
	} // end render function
	
	// 
	this.buildPointerPath = function(value)	{
		var delta = this.config.range / 13;
		
		var head = valueToPoint(value, 0.85);
		var head1 = valueToPoint(value - delta, 0.12);
		var head2 = valueToPoint(value + delta, 0.12);
		
		var tailValue = value - (this.config.range * (1/(270/360)) / 2);
		var tail = valueToPoint(tailValue, 0.28);
		var tail1 = valueToPoint(tailValue - delta, 0.12);
		var tail2 = valueToPoint(tailValue + delta, 0.12);
		
		return [head, head1, tail2, tail, tail1, head2, head];
		
		function valueToPoint(value, factor)
		{
			var point = self.valueToPoint(value, factor);
			point.x -= self.config.cx;
			point.y -= self.config.cy;
			return point;
		}
	} // end buildPointerPath function
	
	// function for configuring normal/warning/danger zone arcs
	// need to add logic for gauges with negative values
	this.drawBand = function(start, end, color) {
		if (0 >= end - start) {							// there cannot be a negatively sized arc (this would mess with the others)
			return;						
		}
		this.body.append("svg:path")						
			 .style("fill", color)						// color the arc appropriately		
			 .attr("d", d3.svg.arc()	
					   // determine arc size (run)
					   .startAngle(this.valueToRadians(start))	// get the arc start point
			 		   .endAngle(this.valueToRadians(end))		// get the arc end point
					   // determine arc width (thickness relative to radius)
			 		   .innerRadius(0.65 * this.config.radius)	// arc outer termination 
			 		   .outerRadius(0.85 * this.config.radius))	// arc outer termination 
			 // rotate the arc to place it where we want it (accounts for the gap on the bottom and other gauge orientation, starting position is at 12 o'clock)
			 .attr("transform", function() { 
				return "translate(" + self.config.cx + ", " + self.config.cy + ") rotate(270)"; 
			 });
	} // end drawBand function
	
	// do animation between prev and curr values (essentially the Animate() function)
	this.redraw = function(value, transitionDuration) {
		var pointerContainer = this.body.select(".pointerContainer");				// only thing being animated is the pointer
		pointerContainer.selectAll("text").text(Math.round(value));				// get the readout (in the pointer container) and round it to integer
		
		var pointer = pointerContainer.selectAll("path");
		pointer.transition()
		       .duration(undefined != transitionDuration ? transitionDuration : this.config.transitionDuration)
		       //.delay(0)
		       //.ease("linear")
		       //.attr("transform", function(d) 
		       .attrTween("transform", function() {
		       		var pointerValue = value;						// determine the current pointer (readout) value
		       		// do "pegging" beyond limits
				if (value > self.config.max) {						// if beyond max, peg the pointer
					pointerValue = self.config.max + 0.02*self.config.range;
				} else if (value < self.config.min) {					// if beyond min, peg the pointer
					pointerValue = self.config.min - 0.02*self.config.range;
				}
				var targetRotation = (self.valueToDegrees(pointerValue) - 90);
				var currentRotation = self._currentRotation || targetRotation;
				self._currentRotation = targetRotation;
				
				return function(step) {
					var rotation = currentRotation + (targetRotation-currentRotation)*step;
					return "translate(" + self.config.cx + ", " + self.config.cy + ") rotate(" + rotation + ")"; 
				}
	               });
	} // end redraw function
	
	// convert value (location along min/max arc) to degrees for mapping
	this.valueToDegrees = function(value) {
		//return value / this.config.range * 270 - 45;
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
			x: this.config.cx - this.config.radius * factor * Math.cos(this.valueToRadians(value)),
			y: this.config.cy - this.config.radius * factor * Math.sin(this.valueToRadians(value)) 		
		};
	} // end valueToPoint function
	
	// initialization
	this.configure(configuration);	
} // end def function Gauge(placeholderName, configuration)