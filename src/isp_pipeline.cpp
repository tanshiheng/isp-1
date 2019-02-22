#include "isp_pipeline.h"
#include "plot.h"

//#define DEBUG_MIPI_RAW

int test_plot();

typedef struct
{
  raw_type_file_dscr_t raw_dscr;
  isp_pra_t *isp_pra;
  raw_ch_t ch;  
  raw_hist_t raw_hsit;
  cv::Mat raw_imag; //fetch raw will return the image    
  cv::Mat blc_imag; //store the img after blc
  cv::Mat lsc_img; //store the img after lsc
}isp_t;

static isp_t *p_isp_server=NULL;

static isp_t *get_isp(void)
{
  return p_isp_server;
}

static int process_fetch_raw(char *name)
{
   int i=0;
   char f_name[128];
   isp_t *p_isp=get_isp();  
   p_isp->raw_imag=fetch_raw(name,&p_isp->raw_dscr); 
   sprintf(f_name,"%s_fetch",name);
   dump_raw_to_png(f_name,p_isp->raw_imag,p_isp->raw_dscr.bayer_format);
}
static int process_get_blc_pra(char *name)
{
   int i=0;   
   cv::Mat raw_imag;  
   isp_t *p_isp=get_isp(); 
   raw_imag=fetch_raw(name,&p_isp->raw_dscr); 
   p_isp->ch=split_raw_ch(raw_imag,p_isp->raw_dscr.bayer_format);
   p_isp->isp_pra->blc_pra=get_blc_value(p_isp->ch,p_isp->raw_dscr.bayer_format);
   save_isp_pra(p_isp->isp_pra,sizeof(isp_pra_t));
}
static int process_proc_blc(char *name)
{
   int i=0;   
   cv::Mat raw_imag; 
   char f_name[128];
   isp_t *p_isp=get_isp();
   p_isp->ch=split_raw_ch(p_isp->raw_imag,p_isp->raw_dscr.bayer_format);
   p_isp->ch=process_blc(p_isp->ch,p_isp->isp_pra->blc_pra,p_isp->raw_dscr.bayer_format);
   merge_raw_ch(p_isp->ch,p_isp->raw_dscr.bayer_format,p_isp->blc_imag); 
   sprintf(f_name,"%s_blc",name);
   dump_raw_to_png(f_name,p_isp->blc_imag,p_isp->raw_dscr.bayer_format);
}
static int process_get_hist(char *name)
{
   int i=0;   
   cv::Mat raw_imag;  
   isp_t *p_isp=get_isp(); 
   raw_imag=fetch_raw(name,&p_isp->raw_dscr); 
   p_isp->ch=split_raw_ch(raw_imag,p_isp->raw_dscr.bayer_format);
   p_isp->raw_hsit=get_raw_hist(128,p_isp->ch,p_isp->raw_dscr.bayer_format);
   save_raw_hist(name,&p_isp->raw_hsit);
   return 0;
}

static int process_plot_raw_hist(char *name)
{ 
   cv::Mat raw_imag;
   cv::Mat ch_img; 
   isp_t *p_isp=get_isp(); 
   raw_imag=fetch_raw(name,&p_isp->raw_dscr); 
   p_isp->ch=split_raw_ch(raw_imag,p_isp->raw_dscr.bayer_format);

   plot_hist(name,"r_hist",get_R_raw(p_isp->ch,p_isp->raw_dscr.bayer_format),0,256<<8);
   plot_hist(name,"gr_hist",get_Gr_raw(p_isp->ch,p_isp->raw_dscr.bayer_format),0,256<<8);
   plot_hist(name,"gb_hist",get_Gb_raw(p_isp->ch,p_isp->raw_dscr.bayer_format),0,256<<8);
   plot_hist(name,"b_hist",get_B_raw(p_isp->ch,p_isp->raw_dscr.bayer_format),0,256<<8);
   return 0;
}

static int get_arg_index_by_name(const char *name, int argc,char *argv[])
{
     int i=0;
     for(i=0;i<argc;i++)
     {
         if(!strcmp(name,argv[i]))
         {
            log_debug("find pra %s @ %d",name,i);
            return i;
         }
     }
     return -1;    
}

static int show_help()
{
   printf("--help :show this help\n");
   printf("--fetch_raw :<name>[raw picture name] \n");
   printf("--get_blc_pra :<name>[blc tuning picture] \n");
   printf("--process :<raw_name> <name1> <name2> ... --end \n");
   printf("--test_plot :test the plat function \n");   
   printf("--get_raw_his :<name>[raw picture] --log_en --dump_mem\n");   
   printf("--plot_raw_hist :<name>[raw picture]\n");
   return 0;
}

int main( int argc, char *argv[])
{
    int i=0;
    isp_t *p_isp=NULL;
    int arg_index=0;    
    char f_name[128];

    for(i=0;i<argc;i++)
    {
       printf("%s ",argv[i]);
    }
    printf("\n");
    
    if(argc==1)
    {
       show_help();
       return 0;
    }
    arg_index=get_arg_index_by_name("--help",argc,argv);
    if(arg_index>0)
    {
        show_help();
        return 0;
    }
    
    p_isp_server=(isp_t *)new(isp_t);
    p_isp=get_isp();
    p_isp->isp_pra=(isp_pra_t *)malloc(sizeof(isp_pra_t));
    #ifdef DEBUG_MIPI_RAW
    p_isp->raw_dscr.width=4208;
    p_isp->raw_dscr.hegiht=3120;
    p_isp->raw_dscr.bitwidth=10;
    p_isp->raw_dscr.is_packed=1;
    p_isp->raw_dscr.height_algin=1;
    p_isp->raw_dscr.width_algin=4;
    p_isp->raw_dscr.line_length_algin=8;
    p_isp->raw_dscr.bayer_format=CV_BayerBG; 
    #else
    #if 1
    p_isp->raw_dscr.width=1952;
    p_isp->raw_dscr.hegiht=1080;
    //p_isp->raw_dscr.hegiht=2288;
    p_isp->raw_dscr.bitwidth=14;
    p_isp->raw_dscr.bitwidth=12;
    p_isp->raw_dscr.is_packed=0;
    p_isp->raw_dscr.height_algin=1;
    p_isp->raw_dscr.width_algin=1;
    p_isp->raw_dscr.line_length_algin=512;
    p_isp->raw_dscr.bayer_format=CV_BayerRG; 
    #else  //dvp raw
    p_isp->raw_dscr.width=1278;
    p_isp->raw_dscr.hegiht=800;
    p_isp->raw_dscr.bitwidth=8;
    p_isp->raw_dscr.is_packed=0;
    p_isp->raw_dscr.height_algin=1;
    p_isp->raw_dscr.width_algin=1;
    p_isp->raw_dscr.line_length_algin=1;
    p_isp->raw_dscr.bayer_format=CV_BayerRG; 
    #endif
    #endif
    cv::Mat blc_imag(p_isp->raw_dscr.hegiht,p_isp->raw_dscr.width,CV_16UC1);
    p_isp->blc_imag=blc_imag;
    
    arg_index=get_arg_index_by_name("--test_plot",argc,argv);
    if(arg_index>0)
    {
       test_plot();
    }
    
    arg_index=get_arg_index_by_name("--fetch_raw",argc,argv);
    if(arg_index>0)
    {
        if((arg_index+1)<=(argc-1)){
           process_fetch_raw(argv[arg_index+1]);           
           sprintf(f_name,"%s_fetch",argv[arg_index+1]);
           show_raw_image(f_name,p_isp->raw_imag,p_isp->raw_dscr.bayer_format);
        }else
        {
           log_err("too less pra for fetch raw");
        }
    }
    arg_index=get_arg_index_by_name("--get_blc_pra",argc,argv);
    if(arg_index>0)
    {
        if((arg_index+1)<=(argc-1)){
           process_get_blc_pra(argv[arg_index+1]);
        }else
        {
           log_err("too less pra for get_blc_pra");
        }
    }

    arg_index=get_arg_index_by_name("--get_raw_his",argc,argv);
    if(arg_index>0)
    {
        int sub_arg_index=0;
        if((arg_index+1)<=(argc-1)){
           process_get_hist(argv[arg_index+1]);
           sub_arg_index=get_arg_index_by_name("--log_en",argc,argv);
           if(sub_arg_index>0)
           {
              log_raw_hist(&p_isp->raw_hsit);
           }
           release_raw_hist(&p_isp->raw_hsit);
        }else
        {
           log_err("too less pra for get_raw_his");
        }
    }

    arg_index=get_arg_index_by_name("--plot_raw_hist",argc,argv);
    if(arg_index>0)
    {
        if((arg_index+1)<=(argc-1)){
           process_plot_raw_hist(argv[arg_index+1]);
        }else
        {
           log_err("too less pra for get_raw_his");
        }
    }
    arg_index=get_arg_index_by_name("--process",argc,argv);
    if(arg_index>0)
    {
       load_isp_pra(p_isp->isp_pra,sizeof(isp_pra_t));
       int arg_end=get_arg_index_by_name("--end",argc,argv);
       if(arg_end>0)
       {
          int pro_index=0;
          process_fetch_raw(argv[arg_index+1]);          
          sprintf(f_name,"%s_fetch",argv[arg_index+1]);
          show_raw_image(argv[arg_index+1],p_isp->raw_imag,p_isp->raw_dscr.bayer_format);
          log_info("arg_index=%d,arg_end=%d",arg_index,arg_end);
          for(pro_index=arg_index+2;pro_index<arg_end;pro_index++)
          {
              if(!strcmp("blc",argv[pro_index]))
              {
                 log_info("%s",argv[pro_index]);
                 process_proc_blc(argv[arg_index+1]);                 
                 sprintf(f_name,"%s_blc",argv[arg_index+1]);
                 show_raw_image(f_name,p_isp->blc_imag,p_isp->raw_dscr.bayer_format);
              }
          }
       }
    }

    log_info("press a key to exit");
    cvWaitKey(0);
    free(p_isp->isp_pra);
    delete(p_isp);
    return 0;
}


