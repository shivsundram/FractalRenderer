__kernel void square2(                                                      
   __global float* input,                                              
   __global int* output,                                        
   const unsigned int count)                                           
{ 

   unsigned int xi = get_global_id(0);  
   unsigned int yi = get_global_id(1);                                         
   if(xi < count && yi<count)                                                      
    {   
	  int xRes = count;    
	  int yRes = count;   

	  //mandelbrot space
	  float x_low=-1.5;
	  float x_high=.8; 
	  float y_low=-1;
	  float y_high=1; 
	  float xlen = x_high - x_low;
	  float ylen = y_high - y_low;

	  int maxiter = 1000; 
	      int iteration = 0; 
	      float x = 0 ;
	      float y = 0; 
	      float cx = x_low + xi*xlen/xRes;
	      float cy = y_low + yi*ylen/yRes;
	      while(x*x + y*y < 4 && iteration<maxiter){
	        float xtemp = x*x - y*y + cx;
	        y = 2*x*y + cy;
	        x = xtemp; 
	        iteration++; 
	      }
	      output[yi*count+xi] = iteration ;
	  
    }
}                                                            
