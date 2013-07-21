<body>
	<div class='vaser_wrap'>
		<div class='article_name'>
			<h1>VASE<span style='line-height:40px;'> renderer</span></h1>
		</div>
<h2>About</h2>
<p>VASE renderer version 0.4 (VASEr 0.4) is a tessellating library for rendering high quality 2D vector graphics. It is an attempt to address unconventional features in 2d graphics. It is intended for OpenGL 1.1+, but much of the code is API independent C++.</p>

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

<h3>Performant</h3>
<div style='position:relative; overflow:hidden; width:300px; height:250px; float:left;'><img style='position:relative; top:-20px; left:-50px;' src='docs/sample_images/polyline_5.png' /></div>
<p>VASEr is for real time rendering. Tessellation is done on CPU and the triangles are then sent off to GPU for rasterization. VASEr is equally fast regardless of the thickness of lines and the rendering resolution.</p>
<p>Can tessellation be accelerated by GPU? Probably, but I have no plan to do so. The performance enhancement strategy of VASEr is adaptation, that means doing expensive joint processing only when necessary and unperceivablely switch to approximation for thin lines and curves. The decision requires some global information and result in lots of branching, which makes it harder to write GPU programs. But after all, correctness is the first priority and performance is the last.</p>

<h2>"Next generation" 2d graphics?</h2>
<p>Q: Who needs "Per vertex coloring and weighting", "Linear gradient along curve" or "Feathering for brush like effects" in vector graphics? A: Same answer as "who needs typesetting and beautiful fonts on their computers?".</p>
<p>Q: These effects can be achieved by vector graphics package XXX using tool AAA then effect BBB, so who needs VASE renderer? A: Arguably true. But the fundamental change is <b>making vertex a three dimensional entity</b> (position, color, weight), and to <b>supercharge the intrinsic properties of strokes</b>. The idea is, it should be <i>natural</i> to apply colors and thickness to a stroke.</p>

<h2>Status</h2>
<p>VASEr is production ready for conventional use. The unconventional features are largely usable for normal circumstances, but may have defects at extreme cases. In particular, varying thickness is undesirable at acute angles and when weight difference is large. So it is not bullet proof yet. The tessellation code is tediously written for each point for each triangle for each circumstance, because a general outsetting and tessellating algorithm is doomed to be slow.</p>
<p>Currently VASEr is only a programming library and is not used in any vector graphics tool. I do have a plan (a plan only) to develop associated tools.</p>
<p>In any case, sending me any form of encouragement (e.g. great work, thank you, donation) will help.</p>

<h2>Documentation</h2>
<div class='textblock'>
	<a href='getting_started.html'>getting started</a><br>
	<a href='API.html'>API reference</a><br>
	Articles<br>
	<a href='http://artgrammer.blogspot.com/2011/07/drawing-polylines-by-tessellation.html' target='_blank'>Drawing polylines by tessellation.</a><br>
	<a href='http://artgrammer.blogspot.hk/2011/05/drawing-nearly-perfect-2d-line-segments.html'>Drawing nearly perfect 2D line segments in OpenGL</a>
</div>

<h2>License</h2>
<p>Copyright (2011,2013) Tsang Hao Fung (Chris Tsang) tyt2y3@gmail.com</p>
<p>This program "VASE Renderer version 0.4" is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation,  either version 3 of the License, or (at your option) any later version.</p>

<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.</p>

<p>You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <a href='http://www.gnu.org/licenses/lgpl.html'>www.gnu.org</a>.</p>

<h3>Credit</h3>
<ul>
<li>Bezier curve subdivision code is extracted from Anti-Grain Geometry V2.5 Copyright 2002-2006 Maxim Shemanarev [GPL license]</li>
</ul>
</body>
