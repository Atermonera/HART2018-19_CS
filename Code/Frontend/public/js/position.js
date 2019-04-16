/*
 * position.js
 * This program uses three.js to produce a 3D representation of the HART rocket's 
 * position and flight path
 * Programmed by: Rick Menzel (menzelr@oregonstate.edu)
 */

// three.js graphics globals
var camera; 
var scene;
var renderer;
var controls;
var container;

// window constants
var winWidth  = 360;	// 4 css grid squares (90*4)
var winHeight = 540;	// 6 css grid squares (90*6)

// grid constants
var gridSize      = 75;
var gridDivisions = 10;
var targetOffset  = 75.;
var medianOffset  = targetOffset / 2;
var texture;
var loader = new THREE.TextureLoader();

// base axes
var xAxis = new THREE.Vector3(1,0,0);
var yAxis = new THREE.Vector3(0,1,0);
var zAxis = new THREE.Vector3(0,0,1);

// stuff for animation
var stageSeperationFlag = 0;
var sustainerDescentFlag = 0;
var boosterSplashFlag = 0;
var sustainerSplashFlag = 0;

var boosterGeometry;
var boosterMaterial;
var boosterIcon;
var boosterPathMaterial;
var boosterPathGeometry;
var boosterPath;
//var boosterX = 0.;
//var boosterY = 0.;
//var boosterZ = 0.;

var sustainerGeometry;
var sustainerMaterial ;
var sustainerIcon;
var sustainerPathMaterial;
var sustainerPathGeometry;
var sustainerPath;
//var sustainerX = 0.;
//var sustainerY = 0.;
//var sustainerZ = 0.;
 
// do first init/render
positionInit();
//render(); 
animate();

/*
 * graphics setup
 */
function positionInit() {
	
	container = document.getElementById('positionCanvas');
	//document.body.appendChild(container);
	
	// use WebGL rendering engine
  renderer = new THREE.WebGLRenderer();
	// render an area equal to the size of our window; 
	// NOTE: can render at a lower resolution by calling setSize 
	// with false as updateStyle (the third argument). For example, 
	// setSize(window.innerWidth/2, window.innerHeight/2, false) 
	// will render the app at half resolution, given that <canvas> 
	// has 100% width and height.
	renderer.setSize( winWidth, winHeight );
	// add render element to html doc
	container.appendChild( renderer.domElement );
	
	// setup the camera
	camera = new THREE.PerspectiveCamera( 45, 			// FOV
					      winWidth / winHeight, 	          // aspect ratio (this ratio is standard)
					      1, 			                          // near clipping plane
					      500 			                        // far clipping plane (can be up to 10000)
					    );					
	camera.position.set( 150, 150, 150 );				    // camera position
	camera.lookAt( 0, 0, 0 );

	//camera = new THREE.PerspectiveCamera( 60, window.innerWidth / window.innerHeight, 1, 1000 );
	//camera.position.set( 400, 200, 0 );
	
	// setup controls
	controls = new THREE.OrbitControls( camera, renderer.domElement );
	// NOTE: an animation loop is required when either damping or auto-rotation are enabled
	controls.enableDamping = true; 
	controls.autoRotate = false;
	controls.dampingFactor = 0.25;
	controls.screenSpacePanning = false;
	controls.minDistance = 100;
	controls.maxDistance = 500;
	controls.maxPolarAngle = Math.PI / 2;
	
	// create three.js scene
	scene = new THREE.Scene();
	
	/*
	 * plane stuff
	 */
	// median grid stuff
	var medianGridBaseColor   = 0x7f7f00;				                  // dark yellow; see https://www.color-hex.com/color/ffff00
	var medianGridCenterColor = 0xffff00;				                  // yellow
	var medianGrid = new THREE.GridHelper(gridSize, 		          // grid size
                                        gridDivisions, 		      // number of divisions
                                        medianGridCenterColor, 	// center line color
                                        medianGridBaseColor    	// base grid color
                                       );
	medianGrid.translateOnAxis( yAxis, medianOffset );				    // translate to vertical midpoint (~75k ft)
	scene.add( medianGrid );					                            // add median grid to scene

	// target grid stuff
	var targetGridBaseColor   = 0x007f00;				                  // dark green; see https://www.color-hex.com/color/00ff00
	var targetGridCenterColor = 0x00ff00;				                  // green
	var targetGrid = new THREE.GridHelper(gridSize, 		          // grid size
                                        gridDivisions, 		      // number of divisions
                                        targetGridCenterColor, 	// center line color
                                        targetGridBaseColor 	  // base grid color
                                       );
	targetGrid.translateOnAxis( yAxis, targetOffset );				    // translate to ceiling (~150k feet)   
	scene.add( targetGrid );					                            // add base grid to scene

	// ground plane stuff
  texture = loader.load('./img/spaceport_crop_labeled.jpg');            // load the image (image is exactly square)
  texture.wrapS = THREE.RepeatWrapping;                         // do s wrapping 
  texture.wrapT = THREE.RepeatWrapping;                         // do t wrapping
  texture.repeat.set( 1, 1 );                                   // stretch image to fit ground plane
  var groundPlaneMaterial = new THREE.MeshBasicMaterial( { map: texture } );    // use the texture
	var groundPlaneGeometry = new THREE.PlaneGeometry(gridSize,   // ground plane x size (same as target grids)
                                                    gridSize,   // ground plane z size (same as target grids)
                                                    1,          // ground plane x divs (not really important)
                                                    1           // ground plane x divs (not really important)
                                                   );
	//var groundPlaneMaterial = new THREE.MeshBasicMaterial( {color: 0x333333, side: THREE.DoubleSide} );	// grey for now
	var groundPlane = new THREE.Mesh( groundPlaneGeometry, groundPlaneMaterial ); // composite
	//groundPlane.translateOnAxis( yAxis, 0 );
	groundPlane.rotation.x = - 90 * Math.PI / 180;								// rotate to horizontal
	scene.add( groundPlane );                                     // add to scene
	
	/*
         * booster stuff
	 */
	// booster icon
	boosterGeometry = new THREE.SphereGeometry( 2, 8, 8 );				            // SphereGeometry(radius, width segments, height segments)
	boosterMaterial = new THREE.MeshBasicMaterial( {color: 0xffb500} );		    // OSU "Luminance" (https://communications.oregonstate.edu/brand-guide/visual-identity/colors)
	boosterIcon = new THREE.Mesh( boosterGeometry, boosterMaterial );
	scene.add( boosterIcon );                                                 // add to scene
	
	//booster flight path
	boosterPathMaterial = new THREE.LineBasicMaterial( {color: 0xffb500} );	  // OSU "Luminance"
	boosterPathGeometry = new THREE.Geometry();                               // "empty" geometry for now 
	boosterPathGeometry.vertices.push( new THREE.Vector3( 0, 0, 0) );		      // at least one point is required at init
	boosterPath = new THREE.Line( boosterPathGeometry, boosterPathMaterial ); // composite
	scene.add( boosterPath );                                                 // add to scene
	
	/*
   * sustainer stuff
	 */
	// sustainer icon
	sustainerGeometry = new THREE.ConeGeometry( 2, 3, 8 );				            // ConeGeometry(radius, height, radial segments)
	sustainerMaterial = new THREE.MeshBasicMaterial( {color: 0xd73f09} );		  // Beaver Orange (https://communications.oregonstate.edu/brand-guide/visual-identity/colors)
	sustainerIcon = new THREE.Mesh( sustainerGeometry, sustainerMaterial );   // composite
	scene.add( sustainerIcon );                                               // add to scene
	
	// sustainer flight path
	sustainerPathMaterial = new THREE.LineBasicMaterial( {color: 0xd73f09} );	// Beaver Orange
	sustainerPathGeometry = new THREE.Geometry();                             // "empty" geometry for now 
	sustainerPathGeometry.vertices.push( new THREE.Vector3( 0, 0, 0 ) );		  // at least one point is required at init
	sustainerPath = new THREE.Line( sustainerPathGeometry, sustainerPathMaterial );// composite
	scene.add( sustainerPath );                                               // add to scene
	
	
	// show axes DEV ONLY
	// X axis is red; Y axis is green; Z axis is blue
	//var axesHelper = new THREE.AxesHelper( 50 );
	//scene.add( axesHelper );
	
} // end function init()

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

/*
 * simple function to animate the composite scene, essentially acts like a while() loop
 */
 
function animate() {
	
	requestAnimationFrame( animate );
 controls.update(); // only required if controls.enableDamping = true, or if controls.autoRotate = true
	render();
}
/*
	if (!stageSeperationFlag) {
		boosterIcon.position.x += .01;
		boosterIcon.position.y += .09;
		boosterIcon.position.z += .01;
		
	        addBoosterPathStep( boosterIcon.position.x, 
		                    boosterIcon.position.y, 
				    boosterIcon.position.z
				  )
	  
		sustainerIcon.position.x += .01;
		sustainerIcon.position.y += .09;
		sustainerIcon.position.z += .01;
		
		addSustainerPathStep( sustainerIcon.position.x, 
		                      sustainerIcon.position.y, 
				      sustainerIcon.position.z
				    )
	  
		if (boosterIcon.position.y >= medianOffset) {
			stageSeperationFlag = 1;
			boosterIcon.position.y -= .09;
		}
	  
	} else if (!sustainerDescentFlag && !boosterSplashFlag && !sustainerSplashFlag) {
		boosterIcon.position.x += .03;
		boosterIcon.position.y -= .09;
		boosterIcon.position.z += .03;
		
		addBoosterPathStep( boosterIcon.position.x, 
		                    boosterIcon.position.y, 
				    boosterIcon.position.z
				  )
		
		sustainerIcon.position.x += .01;
		sustainerIcon.position.y += .09;
		sustainerIcon.position.z += .01;
		
		addSustainerPathStep( sustainerIcon.position.x, 
		                      sustainerIcon.position.y, 
				      sustainerIcon.position.z
				    )
		
		if (boosterIcon.position.y <= 1) {
			boosterSplashFlag = 1;
			boosterIcon.position.y = 1;
		}
		if (sustainerIcon.position.y >= targetOffset) {
			sustainerDescentFlag = 1;
			sustainerIcon.position.y -= .09;
		}	
	} else if (!sustainerDescentFlag && !sustainerSplashFlag) {
		sustainerIcon.position.x += .01;
		sustainerIcon.position.y += .09;
		sustainerIcon.position.z += .01;
		
		addSustainerPathStep( 
                          sustainerIcon.position.x, 
		                      sustainerIcon.position.y, 
                          sustainerIcon.position.z
                        )
		
		if (sustainerIcon.position.y >= targetOffset) {
			sustainerDescentFlag = 1;
			sustainerIcon.position.y -= .09;
		}
	} else if (!sustainerSplashFlag) {
		sustainerIcon.position.x += .03;
		sustainerIcon.position.y -= .09;
		sustainerIcon.position.z += .03;
		
		addSustainerPathStep( 
                          sustainerIcon.position.x, 
		                      sustainerIcon.position.y, 
                          sustainerIcon.position.z
                        );
		
		if (sustainerIcon.position.y <= 1) {
			sustainerSplashFlag = 1;
			sustainerIcon.position.y = 1;
		}
	} 
	
	
	controls.update(); // only required if controls.enableDamping = true, or if controls.autoRotate = true
	render();

}
*/
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			
			
/*
 * simple function to draw the composite scene
 */
function render() {
	
	renderer.render( scene, camera );
	
} // end function render()

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

/*
 * function to update the booster flight history path in the animation loop (roughly based on example http://jsfiddle.net/theo/u6y4n67q/)
 * input: x, y, z values for new point on path
 */
function addBoosterPathStep(x, y, z) {

	vertices = boosterPathGeometry.vertices;					// get the history
	vertices.push( new THREE.Vector3( x, y, z ) );		// add the new point

	boosterPathGeometry = new THREE.Geometry();				// create a new geometry object
	boosterPathGeometry.vertices = vertices;					// append the updated vertices list

	scene.remove( boosterPath );						        	// discard the old flight path
	boosterPath = new THREE.Line( boosterPathGeometry, boosterPathMaterial ); 	// instantiate the new, updated flight path
	scene.add( boosterPath );                         // and finally add it to the scene

}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

/*
 * function to update the booster flight history path in the animation loop (roughly based on example http://jsfiddle.net/theo/u6y4n67q/)
 * input: x, y, z values for new point on path
 */
function addSustainerPathStep(x, y, z) {

	vertices = sustainerPathGeometry.vertices; 					// get the history
	vertices.push( new THREE.Vector3( x, y, z ) );			// add the new point

	sustainerPathGeometry = new THREE.Geometry();				// create a new geometry object
	sustainerPathGeometry.vertices = vertices;					// append the updated vertices list

	scene.remove( sustainerPath );							        // discard the old flight path
	sustainerPath = new THREE.Line( sustainerPathGeometry, sustainerPathMaterial );	// instantiate the new, updated flight path
	scene.add( sustainerPath );                         // and finally add it to the scene

}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@