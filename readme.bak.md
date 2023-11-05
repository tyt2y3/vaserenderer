> This project was started 10 years ago. While the original idea was neat, the implementation was perhaps obscure. It is now being reimplemented in C# for Unity under [tyt2y3/vaser-unity](https://github.com/tyt2y3/vaser-unity).

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

<h3>Performant</h3>
<div style='position:relative; overflow:hidden; width:300px; height:250px; float:left;'><img style='position:relative; top:-20px; left:-50px;' src='docs/sample_images/polyline_5.png' /></div>
<p>VASEr is for real time rendering. Tessellation is done on CPU and the triangles are then sent off to GPU for rasterization. VASEr is equally fast regardless of the thickness of lines and the rendering resolution.</p>

<h2>Documentation</h2>
<div class='textblock'>
	<a href='docs/getting_started.html'>getting started</a><br>
	<a href='docs/API.html'>API reference</a><br>
	Articles<br>
	<a href='http://artgrammer.blogspot.com/2011/07/drawing-polylines-by-tessellation.html' target='_blank'>Drawing polylines by tessellation.</a><br>
	<a href='http://artgrammer.blogspot.hk/2011/05/drawing-nearly-perfect-2d-line-segments.html'>Drawing nearly perfect 2D line segments in OpenGL</a>
</div>

<h2>Related Work</h2>
<ul>
<li>If you only need uniform color & width and only wanted a clean mesh, you can try out <a href="https://github.com/CrushedPixel/Polyline2D">Polyline2D</a>.</li>
</ul>

<h2>License</h2>
<p>VASE renderer version 0.42 (VASEr 0.42) is licensed under The 3-Clause BSD License.</p>
<p>Copyright (2011-2016) Tsang Hao Fung (Chris Tsang) tyt2y3@gmail.com</p>
<p>Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:</p>

<p>1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.</p>

<p>2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.</p>

<p>3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.</p>

<p>THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.</p>

<p>Bezier curve subdivision code is extracted from Anti-Grain Geometry V2.4 Copyright 2002-2005 Maxim Shemanarev (McSeem) [Modified BSD License]</p>

</body>
