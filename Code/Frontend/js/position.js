/*
 *
 *
 *
 */

// three.js graphics globals
var camera; 
var scene;
var renderer;
var controls;
var container;

// window constants
var winWidth  = 360;	// 2 grid cubes (180+180)
var winHeight = 540;	// 2.5 grid cubes (180+180+90)

// grid constants
var gridSize = 80;
var gridDivisions = 10;

// base axes
var xAxis = new THREE.Vector3(1,0,0);
var yAxis = new THREE.Vector3(0,1,0);
var zAxis = new THREE.Vector3(0,0,1);
 
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
					      winWidth / winHeight, 	// aspect ratio (this ratio is standard)
					      1, 			// near clipping plane
					      500 			// far clipping plane (can be up to 10000)
					    );					
	camera.position.set( 150, 150, 150 );							// camera position
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
	
	// median grid stuff
	var medianGridBaseColor   = 0x7f7f00;				// dark yellow; see https://www.color-hex.com/color/ffff00
	var medianGridCenterColor = 0xffff00;				// yellow
	var medianGrid = new THREE.GridHelper( gridSize, 		// grid size
					       gridDivisions, 		// number of divisions
				               medianGridCenterColor, 	// center line color
				               medianGridBaseColor    	// base grid color
					     );
	medianGrid.translateOnAxis( yAxis, 40 );				     
	scene.add( medianGrid );					// add median grid to scene

	// target grid stuff
	var targetGridBaseColor   = 0x007f00;				// dark green; see https://www.color-hex.com/color/00ff00
	var targetGridCenterColor = 0x00ff00;				// green
	var targetGrid = new THREE.GridHelper( gridSize, 		// grid size
                                               gridDivisions, 		// number of divisions
				               targetGridCenterColor, 	// center line color
				               targetGridBaseColor 	// base grid color
				             );
	targetGrid.translateOnAxis( yAxis, 80 );				     
	scene.add( targetGrid );					// add base grid to scene

	// ground plane stuff
	var groundPlaneGeometry = new THREE.PlaneGeometry( 80, 80, 10, 10 );
	var groundPlaneMaterial = new THREE.MeshBasicMaterial( {color: 0x333333, side: THREE.DoubleSide} );
	var groundPlane = new THREE.Mesh( groundPlaneGeometry, groundPlaneMaterial );
	//groundPlane.translateOnAxis( yAxis, 0 );
	groundPlane.rotation.x = - 90 * Math.PI / 180;			// rotate to horizontal
	scene.add( groundPlane );
	
	// booster stuff

	// sustainer stuff

	// show axes DEV ONLY
	// X axis is red; Y axis is green; Z axis is blue
	//var axesHelper = new THREE.AxesHelper( 50 );
	//scene.add( axesHelper );
	
} // end function init()

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

/*
 * simple function to animate the composite scene
 */
function animate() {
	
	requestAnimationFrame( animate );
	controls.update(); // only required if controls.enableDamping = true, or if controls.autoRotate = true
	render();

}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			
			
/*
 * simple function to draw the composite scene
 */
function render() {
	
	renderer.render( scene, camera );
	
} // end function render()

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

/*
 * 
 */
//function onDocumentMouseDown( event ) {

	

//} // end function onDocumentMouseDown()

