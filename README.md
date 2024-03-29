> This project was started many, many years ago. The C++ implementation is a pre-OpenGL 3 relic. The C# implementation is more "modern" and uses fragment shader, which is better.

<body>
	<div class='vaser_wrap'>
		<div class='article_name'>
			<h1>VASE<span style='line-height:40px;'> renderer</span></h1>
		</div>
<h2>About</h2>
<p>VASE renderer version 0.42 (VASEr 0.42) is a tessellating library for rendering high quality 2D vector graphics. It is an attempt to address unconventional features in 2d graphics. It is intended for OpenGL 1.1+, but much of the code is API independent C++.</p>

<h2>Unconventional features</h2>
<h3>Per vertex coloring and weighting</h3>
<div style='position:relative; overflow:hidden; height:220px; float:left;'><img style='position:relative; top:-40px;' src='docs/sample_images/polyline_3.png' /></div>
<br><br>VASEr revolutionarily lets you control the color and thickness at each vertex for a polyline. This feature is unseen on any commonly available graphics library.
	<div style='clear:both'></div>

<h3>Linear gradient along curve</h3>
<div style='position:relative; overflow:hidden; height:200px; float:right;'><img style='position:relative; top:-60px;' src='docs/sample_images/bezier_2.png' /></div>
<br>A similar feature is also available to curves. Because there are so many vertices on a curve that it is impractical to specify data on each point, VASEr lets you define a linear gradient of color and thickness along the length of a curve, giving you two more degrees of freedom.
	<div style='clear:both'></div>

<h3>Feathering for brush like effects</h3>
<img src='docs/sample_images/bezier_3.png' style='float:left;' />
<img src='docs/sample_images/bezier_4.png' style='float:left;' />
<img src='docs/sample_images/bezier_5.png' style='float:left;' />
<br>VASEr must use an outsetting polygon anyway, so it is free (as in computational cost) to scale it up, and it is called feathering in VASEr.
	<div style='clear:both'></div>

<h3>Premium quality anti aliasing</h3>
<div style='background:#333; margin: 10px; border:5px solid #AAA; float:left; height: 150px; overflow:hidden;'>
<img src='docs/sample_images/fade_intro_1.png' />
<img src='docs/sample_images/fade_intro_2.png' />
<img src='docs/sample_images/fade_intro_3.png' />
<img src='docs/sample_images/fade_intro_4.png' />
</div>
<br><br>From left to right: raw polygon without anti aliasing, <br>
anti aliased with outset fade polygon, <br>
exaggerated outsetting polygon with wireframe, <br>
same as left without wireframe.
	<div style='clear:both'></div>
<p>Outset-fade polygon is the high quality, fast, portable, consistent and hassle free technique for anti aliasing. The difficulties are to calibrate the outsetting distance to achieve believable result and to perform tedious tessellation. Luckily VASEr did this for you.</p>
<p>Below are line rendering comparison between VASEr and Cairo and AGG:</p>
<div>
	<div id='ab_va'>VASEr<br><img src='docs/sample_images/ab_vaser_line_thickness1.png' /></div>
	<div id='ab_ca' style='display:none;'>Cairo<br><img src='docs/sample_images/ab_cairo_line_thickness.png' /></div>
</div>
<div>
	<div id='a_agg' style='display:none;'>AGG<br><img src='docs/sample_images/agg_line_thickness.png' /></div>
</div>
<p>There is small difference, but it is more a matter of taste than correctness. In terms of clarity VASEr is like between Cairo and AGG, and VASEr is the most crisp among the three. VASEr even do pixel alignment to 1px completely horizontal or vertical lines.</p>
<p>The flaw is, because the outsetting distance is resolution dependent, while the triangulation is unaffected under rotational transformation, fidelity will be lost under scale and sheer.</p>

<h2>Articles</h2>

+ <a href='http://artgrammer.blogspot.com/2011/07/drawing-polylines-by-tessellation.html' target='_blank'>Drawing polylines by tessellation.</a><br>
+ <a href='http://artgrammer.blogspot.hk/2011/05/drawing-nearly-perfect-2d-line-segments.html'>Drawing nearly perfect 2D line segments in OpenGL</a>

<h2>Related Work</h2>
<ul>
<li>If you only need uniform color & width and only wanted a clean mesh, you can try out <a href="https://github.com/CrushedPixel/Polyline2D">Polyline2D</a>.</li>
</ul>

<h2>Acknowledgement</h2>
<p>Bezier curve subdivision code is extracted from Anti-Grain Geometry V2.4 by Maxim Shemanarev (McSeem).</p>
<p>The open-source graphics library AGG inspired me a lot during my teenage.</p>

</body>
