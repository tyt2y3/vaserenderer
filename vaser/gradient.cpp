namespace VASErin
{	//VASEr internal namespace

double grad_getstep(double A,double B,double t,double T)
{
	return ((T-t)*A + t*B) / T;
}

void gradient_apply(const gradient* gradp, Color* C, double* W, const double* L, int length, double path_length)
{
	if( !gradp) return;
	if( !gradp->stops || !gradp->length) return;
	const gradient& grad = *gradp;
	const gradient_stop* stop = grad.stops;

	//current stops
	short las_c=-1, las_a=-1, las_w=-1, //last
		cur_c=-1, cur_a=-1, cur_w=-1, //current
		nex_c=0, nex_a=0, nex_w=0; //next
	double length_along=0.0;
	if( grad.length>1)
	for( int i=0; i<length; i++)
	{
		length_along += L[i];
		double p;
		if( grad.unit==GD_ratio)
			p = length_along/path_length;
		else if ( grad.unit==GD_length)
			p = length_along;

		{	//lookup for cur
			for( nex_w=cur_w; nex_w < grad.length; nex_w++)
				if( stop[nex_w].type==GS_weight && p <= stop[nex_w].t)
					{ cur_w=nex_w; break;}
			for( nex_c=cur_c; nex_c < grad.length; nex_c++)
				if( (stop[nex_c].type==GS_rgba || stop[nex_c].type==GS_rgb ) &&
					p <= stop[nex_c].t)
					{ cur_c=nex_c; break;}
			for( nex_a=cur_a; nex_a < grad.length; nex_a++)
				if( (stop[nex_a].type==GS_rgba || stop[nex_a].type==GS_alpha ) &&
					p <= stop[nex_a].t)
					{ cur_a=nex_a; break;}
			//look for las
			for( nex_w=cur_w; nex_w >= 0; nex_w--)
				if( stop[nex_w].type==GS_weight && p >= stop[nex_w].t)
					{ las_w=nex_w; break;}
			for( nex_c=cur_c; nex_c >= 0; nex_c--)
				if( (stop[nex_c].type==GS_rgba || stop[nex_c].type==GS_rgb ) &&
					p >= stop[nex_c].t)
					{ las_c=nex_c; break;}
			for( nex_a=cur_a; nex_a >= 0; nex_a--)
				if( (stop[nex_a].type==GS_rgba || stop[nex_a].type==GS_alpha ) &&
					p >= stop[nex_a].t)
					{ las_a=nex_a; break;}
		}
		#define SC(x) stop[x].color
		#define Sw(x) stop[x].weight
		#define Sa(x) stop[x].color.a
		#define St(x) stop[x].t
		if ( cur_c==las_c)
			C[i] = SC(cur_c);
		else
			C[i] = Color_between(SC(las_c),SC(cur_c), (p-St(las_c))/(St(cur_c)-St(las_c)));
		if ( cur_w==las_w)
			W[i] = Sw(cur_w);
		else
			W[i] = grad_getstep(Sw(las_w),Sw(cur_w), p-St(las_w), St(cur_w)-St(las_w));
		if ( cur_a==las_a)
			C[i].a = Sa(cur_a);
		else
			C[i].a = grad_getstep(Sa(las_a),Sa(cur_a), p-St(las_a), St(cur_a)-St(las_a));
		#undef SC
		#undef Sw
		#undef Sa
		#undef St
	}
}

} //sub namespace VASErin
