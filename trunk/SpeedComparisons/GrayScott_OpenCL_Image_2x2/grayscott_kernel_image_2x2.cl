__kernel void grayscott_compute_2x2(
    read_only image2d_t a, 
    read_only image2d_t b,
    write_only image2d_t a2, 
    write_only image2d_t b2,
    float f,float f_plus_k,
    float r_a,float r_b,
    float speed)
{
    const sampler_t smp = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_REPEAT | CLK_FILTER_NEAREST;

    const int x = get_global_id(0);
    const int y = get_global_id(1);

    const float4 a_pixel = read_imagef(a, smp, (int2)(x, y));
    const float4 a_left = read_imagef(a, smp, (int2)(x-1, y));
    const float4 a_right = read_imagef(a, smp, (int2)(x+1, y));
    const float4 a_up = read_imagef(a, smp, (int2)(x, y-1));
    const float4 a_down = read_imagef(a, smp, (int2)(x, y+1));
    const float4 b_pixel = read_imagef(b, smp, (int2)(x, y));
    const float4 b_left = read_imagef(b, smp, (int2)(x-1, y));
    const float4 b_right = read_imagef(b, smp, (int2)(x+1, y));
    const float4 b_up = read_imagef(b, smp, (int2)(x, y-1));
    const float4 b_down = read_imagef(b, smp, (int2)(x, y+1));
 
    // we pack the values in 2x2 blocks:   x y
    //                                     z w

    write_imagef( a2, (int2)(x, y), a_pixel + speed * (r_a * ((float4)(
        a_left.y + a_up.z + a_pixel.y + a_pixel.z,
        a_pixel.x + a_up.w + a_right.x + a_pixel.w,
        a_left.w + a_pixel.x + a_pixel.w + a_down.x,
        a_pixel.z + a_pixel.y + a_right.z + a_down.y) - 4*a_pixel) 
        - a_pixel * b_pixel * b_pixel + f*(1-a_pixel)) );
    write_imagef( b2, (int2)(x, y), b_pixel + speed * (r_b * ((float4)(
        b_left.y + b_up.z + b_pixel.y + b_pixel.z,
        b_pixel.x + b_up.w + b_right.x + b_pixel.w,
        b_left.w + b_pixel.x + b_pixel.w + b_down.x,
        b_pixel.z + b_pixel.y + b_right.z + b_down.y) - 4*b_pixel) 
        + a_pixel * b_pixel * b_pixel - f_plus_k*b_pixel) );
    // (it's faster than using local const floats to split up the computation, annoyingly)
}
