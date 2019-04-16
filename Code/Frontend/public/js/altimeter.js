/*
 * altimeter.js
 * This program uses d3.js built-ins and a library called iopctrl to produce altimeter 
 * and GPS readouts with a digital clock-like appearance. Note that colors are set 
 * separately using CSS.
 * Programmed by: Rick Menzel (menzelr@oregonstate.edu)
 */
 
 var readouts = [];
 
 //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 
 /*
 * simple helper function to create a readout with specified params
 */
function createReadout(name, width, height, digitCount, decimals, negative) {
	var config = {
		width: width,
		height: height,
		digitCount: digitCount,
    decimals: decimals,
    negative: negative
	}
	
	readouts[name] = new Readout(name, config);
	readouts[name].render();
} // end function createReadout()

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

/*
 * simple helper function to invoke readout creator fn
 */
function createReadouts() {
	createReadout("apogee",             200, 55, 6, 0, false);  // 0
	createReadout("boosterAltimeter",   110, 28, 6, 0, false);	// 1
	createReadout("sustainerAltimeter", 110, 28, 6, 0, false);	// 2
	createReadout("boosterLat",         150, 28, 9, 6, true);	  // 3
	createReadout("boosterLon",         150, 28, 9, 6, true);	  // 4
	createReadout("sustainerLat",       150, 28, 9, 6, true);	  // 5
  createReadout("sustainerLon",       150, 28, 9, 6, true);   // 6
  createReadout("clock",              110, 34, 5, 2, false);  // 7
} // end function createReadouts()

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

/*
 * simple helper function to create readouts - called by html on page load
 */	
function initReadouts() {
	createReadouts();
} // end function initReadouts()

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

/*
 * primary graphics function for segmented digital style readouts
 */
function Readout(readoutName, readoutConfig) {
  this.readoutName = readoutName;
  var self = this;                                                // this needs to be done so the redraw function can access the segdisplay value
  
  // config function. sets various readout parameters
  this.configure = function(readoutConfig) {
    this.config            = readoutConfig;
    this.config.width      = this.config.width      || 110;
    this.config.height     = this.config.height     || 28;
    this.config.digitCount = this.config.digitCount || 6;
    this.config.decimals   = this.config.decimals   || 0;
    this.config.negative   = this.config.negative   || false;
  }
  
  // graphics display function
  this.render = function() {
    this.segDisplay = iopctrl.segdisplay()                        // set up the seg display (d3 element must contain svg or g element)
                             .width(this.config.width)				    // absolute element width
                             .digitCount(this.config.digitCount)	// number of display digits
                             .decimals(this.config.decimals)			// number of display decimal values
                             .negative(this.config.negative);			// accepts negative values?
    this.body = d3.select("#" + this.readoutName + "Container")
                  .append("svg:svg")				                      // append scalable vector graphics attribute
                  .attr("class", this.readoutName + "Display")	  // for css styling
                  .attr("width", this.config.width)			          // svg absolute width
                  .attr("height", this.config.height)	            // svg absolute height
                  .append("g")                                    // for iopctrl
                  .call(this.segDisplay);                         // invoke the segDisplay
    this.redraw(0.0);              
  }
  // animation function. call this to update the readout
  this.redraw = function(val) {
    self.segDisplay.value(val);
  }
  // actually do the readout config
  this.configure(readoutConfig);
   
 }