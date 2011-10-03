__kernel void grayscott_compute (
    read_only image2d_t input, 
    write_only image2d_t output,
    float f,float f_plus_k,
    float r_a,float r_b,
    float speed)
{
    const sampler_t smp = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_REPEAT | CLK_FILTER_NEAREST;

    const int x = get_global_id(0);
    const int y = get_global_id(1);

    const float4 pixel = read_imagef(input, smp, (int2)(x, y));
    const float4 left = read_imagef(input, smp, (int2)(x-1, y));
    const float4 right = read_imagef(input, smp, (int2)(x+1, y));
    const float4 up = read_imagef(input, smp, (int2)(x, y-1));
    const float4 down = read_imagef(input, smp, (int2)(x, y+1));
 
    const float4 laplacian = left + right + up + down - 4*pixel;

    {    
        // for Gray-Scott we only use the first two components:
        
        const float da = r_a * laplacian.x - pixel.x*pixel.y*pixel.y + f*(1-pixel.x);
        const float db = r_b * laplacian.y + pixel.x*pixel.y*pixel.y - f_plus_k*pixel.y;
        
        pixel.x += speed * da;
        pixel.y += speed * db;
    }

    write_imagef(output, (int2)(x, y), pixel);
}