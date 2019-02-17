/*
 *
 *
 *
 */
 
/*
 * apogee readout
 */
// setup element
var apogeeSvg = d3.select("#apogeeContainer")			// get the display element
                   .append("svg:svg")				// append scalable vector graphics attribute
                   .attr("width", 200)				// svg absolute width
                   .attr("height", 55);				// svg absolute height
// create segmented "digital" style display
var apogeeDisplay = iopctrl.segdisplay()
                           .width(200)		// absolute element width
                           .digitCount(6)	// number of display digits
                           .negative(false)	// accepts negative values?
                           .decimals(0);	// number of display decimal values
// attach segmented "digital" style display to document
apogeeSvg.append("g")
   .attr("class", "apogeeDisplay")		// for css styling
   //.attr("transform", "translate(200, 0)")	// position display within total element
   .call(apogeeDisplay);			// invoke the readout

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			   
/*
 * booster readouts
 */ 
// setup stage altitude readouts
var boosterSvg = d3.select("#boosterAltimeterContainer")	// get the display element
                   .append("svg:svg")				// append scalable vector graphics attribute
                   .attr("width", 100)				// svg absolute width
                   .attr("height", 27);				// svg absolute height
// setup latitude and longitude readouts
var boosterLatSvg = d3.select("#boosterLatContainer")	// get the display element
                   .append("svg:svg")				// append scalable vector graphics attribute
                   .attr("width", 100)				// svg absolute width
                   .attr("height", 22);				// svg absolute height
var boosterLonSvg = d3.select("#boosterLonContainer")	// get the display element
                   .append("svg:svg")				// append scalable vector graphics attribute
                   .attr("width", 100)				// svg absolute width
                   .attr("height", 22);				// svg absolute height
// segmented style digital displays
var boosterDisplay = iopctrl.segdisplay()
                           .width(100)		// absolute element width
                           .digitCount(6)	// number of display digits
                           .negative(false)	// accepts negative values?
                           .decimals(0);	// number of display decimal values
var boosterLatDisplay = iopctrl.segdisplay()
                           .width(100)		// absolute element width
                           .digitCount(8)	// number of display digits
                           .negative(false)	// accepts negative values?
                           .decimals(6);	// number of display decimal values
var boosterLonDisplay = iopctrl.segdisplay()
                           .width(100)		// absolute element width
                           .digitCount(8)	// number of display digits
                           .negative(false)	// accepts negative values?
                           .decimals(6);	// number of display decimal values
// attach segmented "digital" style displays to document
boosterSvg.append("g")
   .attr("class", "boosterDisplay")		// for css styling
   //.attr("transform", "translate(200, 0)")	// position display within total element
   .call(boosterDisplay);			// invoke the readout
boosterLatSvg.append("g")
   .attr("class", "boosterLatDisplay")		// for css styling
   //.attr("transform", "translate(200, 0)")	// position display within total element
   .call(boosterLatDisplay);			// invoke the readout
boosterLonSvg.append("g")
   .attr("class", "boosterLonDisplay")		// for css styling
   //.attr("transform", "translate(200, 0)")	// position display within total element
   .call(boosterLonDisplay);			// invoke the readout   
			   
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		   
/*
 * sustainer readouts
 */ 	
// setup stage altitude readouts 
var sustainerSvg = d3.select("#sustainerAltimeterContainer")	// get the display element
                   .append("svg:svg")				// append scalable vector graphics attribute
                   .attr("width", 100)				// svg absolute width
                   .attr("height", 27);				// svg absolute height
// setup latitude and longitude readouts
var sustainerLatSvg = d3.select("#sustainerLatContainer")	// get the display element
                   .append("svg:svg")				// append scalable vector graphics attribute
                   .attr("width", 100)				// svg absolute width
                   .attr("height", 22);				// svg absolute height
var sustainerLonSvg = d3.select("#sustainerLonContainer")	// get the display element
                   .append("svg:svg")				// append scalable vector graphics attribute
                   .attr("width", 100)				// svg absolute width
                   .attr("height", 22);				// svg absolute height
// segmented style digital displays
var sustainerDisplay = iopctrl.segdisplay()
                           .width(100)		// absolute element width
                           .digitCount(6)	// number of display digits
                           .negative(false)	// accepts negative values?
                           .decimals(0);	// number of display decimal values
var sustainerLatDisplay = iopctrl.segdisplay()
                           .width(100)		// absolute element width
                           .digitCount(8)	// number of display digits
                           .negative(false)	// accepts negative values?
                           .decimals(6);	// number of display decimal values
var sustainerLonDisplay = iopctrl.segdisplay()
                           .width(100)		// absolute element width
                           .digitCount(8)	// number of display digits
                           .negative(false)	// accepts negative values?
                           .decimals(6);	// number of display decimal values
// attach segmented "digital" style displays to document
sustainerSvg.append("g")
   .attr("class", "sustainerDisplay")		// for css styling
   //.attr("transform", "translate(200, 0)")	// position display within total element
   .call(sustainerDisplay);			// invoke the readout   
sustainerLatSvg.append("g")
   .attr("class", "sustainerLatDisplay")		// for css styling
   //.attr("transform", "translate(200, 0)")	// position display within total element
   .call(sustainerLatDisplay);			// invoke the readout
sustainerLonSvg.append("g")
   .attr("class", "sustainerLonDisplay")		// for css styling
   //.attr("transform", "translate(200, 0)")	// position display within total element
   .call(sustainerLonDisplay);			// invoke the readout   
     			
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	
/*
 * set readout values
 */
apogeeDisplay.value(150000);			
boosterDisplay.value(256);		
sustainerDisplay.value(17000);		
boosterLatDisplay.value(44.000123);
boosterLonDisplay.value(44.000123);
sustainerLatDisplay.value(44.000123);
sustainerLonDisplay.value(44.000123);