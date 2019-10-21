/*namespace Vaser
{
    public class Polyline
    {
        private struct st_anchor
        //the struct to hold memory for the working of anchor()
        {
            Point P[3]; //point
            Color C[3]; //color
            double W[3];//weight

            Point cap_start, cap_end;
            st_polyline SL[3];
            vertex_array_holder vah;
        }

        public static void Anchor( st_anchor SA, const polyline_opt* options, bool cap_first, bool cap_last)
        {
            polyline_opt opt={0};
            if ( options)
                opt = (*options);
            
            Point* P = SA.P;
            Color* C = SA.C;
            double* weight = SA.W;

            {   st_polyline emptySL;
                SA.SL[0]=emptySL; SA.SL[1]=emptySL; SA.SL[2]=emptySL;
            }
            st_polyline* SL = SA.SL;
            SA.vah.set_gl_draw_mode(GL_TRIANGLES);
            SA.cap_start = Point();
            SA.cap_end = Point();
            
            //const double critical_angle=11.6538;
            //  critical angle in degrees where a miter is force into bevel
            //  it is _similar_ to cairo_set_miter_limit () but cairo works with ratio while VASEr works with included angle
            const double cos_cri_angle=0.979386; //cos(critical_angle)
            
            bool varying_weight = !(weight[0]==weight[1] & weight[1]==weight[2]);
            
            double combined_weight=weight[1]+(opt.feather?opt.feathering:0.0);
            if ( combined_weight < cri_segment_approx)
            {
                segment( SA, &opt, cap_first,false, opt.joint==PLJ_round?PLC_round:PLC_butt);
                char ori_cap = opt.cap;
                opt.cap = opt.joint==PLJ_round?PLC_round:PLC_butt;
                SA.P[0]=SA.P[1]; SA.P[1]=SA.P[2];
                SA.C[0]=SA.C[1]; SA.C[1]=SA.C[2];
                SA.W[0]=SA.W[1]; SA.W[1]=SA.W[2];
                segment( SA, &opt, false,cap_last, ori_cap);
                return 0;
            }
            
            Point T1,T2,T21,T31;        //]these are for calculations in early stage
            Point R1,R2,R21,R31;        //]
            
            for ( int i=0; i<3; i++)
            {   //lower the transparency for weight < 1.0
                if ( weight[i]>=0.0 && weight[i]<1.0)
                {
                    double f=weight[i];
                    C[i].a *=f;
                }
            }
            
            {   int i=0;
            
                Point cap1;
                double r,t;     
                make_T_R_C( P[i], P[i+1], &T2,&R2,&cap1, weight[i],opt, &r,&t,0);
                if ( varying_weight) {
                make_T_R_C( P[i], P[i+1], &T31,&R31,0, weight[i+1],opt, 0,0,0);
                } else {
                    T31 = T2;
                    R31 = R2;
                }
                Point::anchor_outward(R2, P[i+1],P[i+2]);
                    T2.follow_signs(R2);
                
                SL[i].bR=cap1;
                
                if ( cap_first)
                {
                    if ( opt.cap==PLC_square)
                    {
                        P[0] = Point(P[0]) - cap1 * (t+r);
                    }
                    cap1.opposite(); if ( opt.feather && !opt.no_feather_at_cap)
                    cap1*=opt.feathering;
                    SA.cap_start = cap1;
                }
                
                SL[i].djoint=opt.cap;
                SL[i].T=T2;
                SL[i].R=R2;
                SL[i].t=(float)t;
                SL[i].r=(float)r;
                SL[i].degenT = false;
                SL[i].degenR = false;
                
                SL[i+1].T1=T31;
                SL[i+1].R1=R31;
            }
            
            if ( cap_last)
            {   int i=2;

                Point cap2;
                double t,r;
                make_T_R_C( P[i-1],P[i], 0,0,&cap2,weight[i],opt, &r,&t,0);
                if ( opt.cap==PLC_square)
                {
                    P[2] = Point(P[2]) + cap2 * (t+r);
                }
                
                SL[i].bR=cap2;
                
                if ( opt.feather && !opt.no_feather_at_cap)
                    cap2*=opt.feathering;
                SA.cap_end = cap2;
            }
            
            {   int i=1;
            
                double r,t;
                Point P_cur = P[i]; //current point //to avoid calling constructor repeatedly
                Point P_nxt = P[i+1]; //next point
                Point P_las = P[i-1]; //last point
                if ( opt.cap==PLC_butt || opt.cap==PLC_square)
                {
                    P_nxt -= SA.cap_end;
                    P_las -= SA.cap_start;
                }
                
                {
                Point bR; float length_cur, length_nxt;
                make_T_R_C( P_las, P_cur,  &T1,&R1, 0, weight[i-1],opt,0,0, &length_cur);
                if ( varying_weight) {
                make_T_R_C( P_las, P_cur, &T21,&R21,0, weight[i],opt,  0,0,0);
                } else {
                    T21 = T1;
                    R21 = R1;
                }
                
                make_T_R_C( P_cur, P_nxt,  &T2,&R2,&bR, weight[i],opt, &r,&t, &length_nxt);
                if ( varying_weight) {
                make_T_R_C( P_cur, P_nxt, &T31,&R31,0, weight[i+1],opt, 0,0,0);
                } else {
                    T31 = T2;
                    R31 = R2;
                }
                
                SL[i].T=T2;
                SL[i].R=R2;
                SL[i].bR=bR;
                SL[i].t=(float)t;
                SL[i].r=(float)r;
                SL[i].degenT = false;
                SL[i].degenR = false;
                
                SL[i+1].T1=T31;
                SL[i+1].R1=R31;
                }
                
                {   //2nd to 2nd last point
                    
                    //find the angle between the 2 line segments
                    Point ln1,ln2, V;
                    ln1 = P_cur - P_las;
                    ln2 = P_nxt - P_cur;
                    ln1.normalize();
                    ln2.normalize();
                    Point::dot(ln1,ln2, V);
                    double cos_tho=-V.x-V.y;
                    bool zero_degree = Point::negligible(cos_tho-1);
                    bool d180_degree = cos_tho < -1+0.0001;
                    bool smaller_than_30_degree = cos_tho > 0.8660254;
                    char result3 = 1;
                    
                    if ( (cos_tho < 0 && opt.joint==PLJ_bevel) ||
                         (opt.joint!=PLJ_bevel && opt.cap==PLC_round) ||
                         (opt.joint==PLJ_round)
                       )
                    {   //when greater than 90 degrees
                        SL[i-1].bR *= 0.01;
                        SL[i]  .bR *= 0.01;
                        SL[i+1].bR *= 0.01;
                        //to solve an overdraw in bevel and round joint
                    }
                    
                    Point::anchor_outward( T1, P_cur,P_nxt);
                        R1.follow_signs(T1);
                    Point::anchor_outward( T21, P_cur,P_nxt);
                        R21.follow_signs(T21);
                        SL[i].T1.follow_signs(T21);
                        SL[i].R1.follow_signs(T21);
                    Point::anchor_outward( T2, P_cur,P_las);
                        R2.follow_signs(T2);
                        SL[i].T.follow_signs(T2);
                        SL[i].R.follow_signs(T2);
                    Point::anchor_outward( T31, P_cur,P_las);
                        R31.follow_signs(T31);
                    
                    { //must do intersection
                        Point interP, vP;
                        result3 = Point::intersect( P_las+T1, P_cur+T21,
                                    P_nxt+T31, P_cur+T2,
                                    interP);
                        
                        if ( result3) {
                            vP = interP - P_cur;
                            SL[i].vP=vP;
                            SL[i].vR=vP*(r/t);
                        } else {
                            SL[i].vP=SL[i].T;
                            SL[i].vR=SL[i].R;
                            DEBUG( "intersection failed: cos(angle)=%.4f, angle=%.4f(degree)\n", cos_tho, acos(cos_tho)*180/3.14159);
                        }
                    }
                    
                    T1.opposite();      //]inward
                        R1.opposite();
                    T21.opposite();
                        R21.opposite();
                    T2.opposite();
                        R2.opposite();
                    T31.opposite();
                        R31.opposite();
                    
                    //make intersections
                    Point PR1,PR2, PT1,PT2;
                    double pt1,pt2;
                    
                    char result1r = Point::intersect( P_nxt-T31-R31, P_nxt+T31+R31,
                                P_las+T1+R1, P_cur+T21+R21, //knife1
                                PR1); //fade
                    char result2r = Point::intersect( P_las-T1-R1, P_las+T1+R1,
                                P_nxt+T31+R31, P_cur+T2+R2, //knife2
                                PR2);
                    bool is_result1r = result1r == 1;
                    bool is_result2r = result2r == 1;
                    //
                    char result1t = Point::intersect( P_nxt-T31, P_nxt+T31,
                                P_las+T1, P_cur+T21, //knife1_a
                                PT1, 0,&pt1); //core
                    char result2t = Point::intersect( P_las-T1, P_las+T1,
                                P_nxt+T31, P_cur+T2, //knife2_a
                                PT2, 0,&pt2);
                    bool is_result1t = result1t == 1;
                    bool is_result2t = result2t == 1;
                    //
                    bool inner_sec = Point::intersecting( P_las+T1+R1, P_cur+T21+R21,
                                P_nxt+T31+R31, P_cur+T2+R2);
                    //
                    if ( zero_degree)
                    {
                        bool pre_full = is_result1t;
                        opt.no_feather_at_cap=true;
                        if ( pre_full)
                        {
                            segment( SA, &opt, true,cap_last, opt.joint==PLJ_round?PLC_round:PLC_butt);
                        }
                        else
                        {
                            char ori_cap = opt.cap;
                            opt.cap = opt.joint==PLJ_round?PLC_round:PLC_butt;
                            SA.P[0]=SA.P[1]; SA.P[1]=SA.P[2];
                            SA.C[0]=SA.C[1]; SA.C[1]=SA.C[2];
                            SA.W[0]=SA.W[1]; SA.W[1]=SA.W[2];
                            segment( SA, &opt, true,cap_last, ori_cap);
                        }
                        return 0;
                    }
                    
                    if ( (is_result1r | is_result2r) && !inner_sec)
                    {   //fade degeneration
                        SL[i].degenR=true;
                        SL[i].PT = is_result1r? PT1:PT2; //this is is_result1r!!
                        SL[i].PR = is_result1r? PR1:PR2;
                        SL[i].pt = float(is_result1r? pt1:pt2);
                            if ( SL[i].pt < 0)
                                SL[i].pt = cri_core_adapt;
                        SL[i].pre_full = is_result1r;
                        SL[i].R_full_degen = false;
                        
                        Point P_nxt = P[i+1]; //override that in the parent scope
                        Point P_las = P[i-1];
                        Point PR;
                        if ( opt.cap==PLC_rect || opt.cap==PLC_round)
                        {
                            P_nxt += SA.cap_end;
                            P_las += SA.cap_start;
                        }
                        char result2;
                        if ( is_result1r)
                        {
                            result2 = Point::intersect( P_nxt-T31-R31, P_nxt+T31,
                                P_las+T1+R1, P_cur+T21+R21, //knife1
                                PR);    //fade
                        }
                        else
                        {
                            result2 = Point::intersect( P_las-T1-R1, P_las+T1,
                                P_nxt+T31+R31, P_cur+T2+R2, //knife2
                                PR);
                        }
                        if ( result2 == 1)
                        {
                            SL[i].R_full_degen = true;
                            SL[i].PR = PR;
                        }
                    }
                    
                    if ( is_result1t | is_result2t)
                    {   //core degeneration
                        SL[i].degenT=true;
                        SL[i].pre_full=is_result1t;
                        SL[i].PT = is_result1t? PT1:PT2;
                        SL[i].pt = float(is_result1t? pt1:pt2);
                    }
                    
                    //make joint
                    SL[i].djoint = opt.joint;
                    if ( opt.joint == PLJ_miter)
                        if ( cos_tho >= cos_cri_angle)
                            SL[i].djoint=PLJ_bevel;

                    if ( d180_degree | !result3)
                    {   //to solve visual bugs 3 and 1.1
                        //efficiency: if color and weight is same as previous and next point
                        // ,do not generate vertices
                        same_side_of_line( SL[i].R, SL[i-1].R, P_cur,P_las);
                            SL[i].T.follow_signs(SL[i].R);
                        SL[i].vP=SL[i].T;
                        SL[i].T1.follow_signs(SL[i].T);
                        SL[i].R1.follow_signs(SL[i].T);
                        SL[i].vR=SL[i].R;
                        SL[i].djoint=PLJ_miter;
                    }
                } //2nd to 2nd last point
            }
            
            {   int i=2;

                double r,t;
                make_T_R_C( P[i-1],P[i], &T2,&R2,0,weight[i],opt,  &r,&t,0);
                    same_side_of_line( R2, SL[i-1].R, P[i-1],P[i]);
                        T2.follow_signs(R2);
                
                SL[i].djoint=opt.cap;
                SL[i].T=T2;
                SL[i].R=R2;
                SL[i].t=(float)t;
                SL[i].r=(float)r;
                SL[i].degenT = false;
                SL[i].degenR = false;
            }
            
            if( cap_first || cap_last)
                anchor_cap( SA.P,SA.C, SA.SL,SA.vah, SA.cap_start,SA.cap_end);
            anchor_late( SA.P,SA.C, SA.SL,SA.vah, SA.cap_start,SA.cap_end);
            return 1;
        } //anchor
    }
}*/