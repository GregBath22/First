#include <pebble.h>
#include <string.h>
#include <stdio.h>
  
#define KEY_START 0
#define KEY_STOP 1
#define KEY_STORED 2
#define KEY_TRANS_FINISHED 3
#define NUM_SAMPLES 1
#define MEM_SIZE 1500

Window *window;
TextLayer *title, *status;
DataLoggingSessionRef logging_sesh;
int CollectData = 0;
int mem_count = 0;
int begin;
int data_stored = 0;
int transfer = 0;

//FILE* tp = NULL;

static uint64_t latest_data[4 * NUM_SAMPLES];

struct My_Mem
{
  int16_t X;
  int16_t Y;
  int16_t Z;
} *memory;

void set_text(TextLayer *View, const char *newText, const char *font )
{
  // Set the text, font, and text alignment
	text_layer_set_text(View, newText );
	text_layer_set_font(View, fonts_get_system_font(font));
	text_layer_set_text_alignment(View, GTextAlignmentCenter);
	
	// Add the text layer to the window
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(View));
}

void up_click_handler( ClickRecognizerRef recognizer, void *context)
{
  
}

void down_click_handler( ClickRecognizerRef recognizer, void *context)
{
  //reset data storage
  data_stored = 0;
}
void toggle_collecting()
{
  
      if(CollectData == 0)
      {
        CollectData = 1;
        mem_count = 0;
      }
      else if (CollectData == 1)
      {
        CollectData = 0;
        mem_count = 0;
        data_stored = 1;
        set_text(status,"Finished Collecting",FONT_KEY_GOTHIC_24_BOLD);
        return;
      }
}

void select_click_handler( ClickRecognizerRef recognizer, void *context)
{
  toggle_collecting();
}

void select_long_click_handler( ClickRecognizerRef recognizer, void *context)
{
    
    //data_logging_log(logging_sesh,  &test, 4);
}

void click_config_provider(void *context){
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, NULL, select_long_click_handler);
}

static void accel_new_data(AccelData *data, uint32_t num_samples)
{
  
  	for(uint32_t i = 0; i < num_samples; i++)
  	{
  		latest_data[(i * 4) + 0] = (int)(0 + data[i].x);	//0, 3, 6 
  		latest_data[(i * 4) + 1] = (int)(0 + data[i].y);	//1, 4, 7
  		latest_data[(i * 4) + 2] = (int)(0 + data[i].z);	//2, 5, 8
      latest_data[(i * 4) + 3] = (uint64_t)(0 + (data[i].timestamp ));
     //data_logging_log(logging_sesh, &latest_data, 4);
     // set_text(status,"Working",FONT_KEY_GOTHIC_24_BOLD);
  	
      if(CollectData)
      {
      set_text(status,"Collecting",FONT_KEY_GOTHIC_24_BOLD);
      latest_data[(i * 4) + 3] = 0;
      memory[mem_count].X = latest_data[(i * 4) + 0];
      memory[mem_count].Y = latest_data[(i * 4) + 1];
      memory[mem_count].Z = latest_data[(i * 4) + 2];
      mem_count++;
        if( mem_count == (MEM_SIZE-1)) {
        toggle_collecting(); 
        }
     }
    }
}
  

static void in_dropped_handler(AppMessageResult reason, void *context) 
{ 
  set_text(status,"fail",FONT_KEY_BITHAM_42_BOLD);
}

static void out_failed_handler(DictionaryIterator *iter, AppMessageResult result, void *context)
{
  set_text(status,"Connection Lost",FONT_KEY_GOTHIC_24_BOLD);
}

static void send_next_data()
{
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	for(int i = 0; i < NUM_SAMPLES; i++)
	{
		for(int j = 0; j < 4; j++)
		{
			int value = 0 + latest_data[(4 * i) + j];
			Tuplet t = TupletInteger((4 * i) + j, value);
			dict_write_tuplet(iter, &t);
		}
	}
  
	app_message_outbox_send();
}

static void send_stored_data()
{
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
  //int temp;

	//for(int i = 0; i < 3; i++)
	
		  //temp = i;
    
    int valueX = 0 + memory[mem_count].X;
		Tuplet t1 = TupletInteger(4, valueX);
		dict_write_tuplet(iter, &t1);
    int valueY = 0 + memory[mem_count].Y;
    Tuplet t2 = TupletInteger(5, valueY);
		dict_write_tuplet(iter, &t2);
		int valueZ = 0 + memory[mem_count].Z;
		Tuplet t3 = TupletInteger(6, valueZ);
		dict_write_tuplet(iter, &t3);
  
    mem_count++;
    if( mem_count == (MEM_SIZE-1)) {
      transfer = 0;
    }
	
  //if(temp < 10){
	app_message_outbox_send();
  
}

static void out_sent_handler(DictionaryIterator *iter, void *context)
{
	//CAUTION - INFINITE LOOP
	if(transfer == 0){
    send_next_data();
  }
  else if( transfer == 1 && data_stored == 1){
    send_stored_data();
  }
  

	
}

static void process_tuple(Tuple *t)
{
  
	switch(t->key)
	{
	case KEY_START: 
		set_text(status,"Connection established.", FONT_KEY_GOTHIC_24_BOLD);
    transfer = 0;
		send_next_data();
	break;
  case KEY_STOP:
    set_text(status,"Connection stopped.", FONT_KEY_GOTHIC_24_BOLD);
    transfer = 2;
  break;
  case KEY_STORED:
    transfer = 1;
    CollectData = 0;
    mem_count = 0;
    set_text(status,"Transfer in progress.", FONT_KEY_GOTHIC_24_BOLD);
    
    send_stored_data();
    break;
   case KEY_TRANS_FINISHED:
    set_text(status,"Transfer Done.", FONT_KEY_GOTHIC_24_BOLD);
    transfer = 2;
    break;
	}
}


static void in_received_handler(DictionaryIterator *iter, void *context)
{	
	//Get data
  Tuple *t = dict_read_first(iter);
    	if(t)
    	{
    		process_tuple(t);
    	}
    
    	//Get next
    	while(t != NULL) 
    	{
    		t = dict_read_next(iter);
    		if(t)
    		{
    			process_tuple(t);
    		}
    	}  
}


void handle_init(void) 
{
  //Allocation of dynamic memory
  memory = malloc(MEM_SIZE * sizeof(*memory));
  
  if(memory == NULL)
  {
    set_text(status,"Memory Fail", FONT_KEY_GOTHIC_24_BOLD);
  }
  
	// Create a window and text layers
	window = window_create();
  window_set_click_config_provider(window, click_config_provider);
	title = text_layer_create(GRect(0, 0, 144, 90));
  status = text_layer_create(GRect(0,90,144,100));
  logging_sesh = data_logging_create(0x1234, DATA_LOGGING_UINT, 4, false);
  
  CollectData = 0;
  // Setup App to use accelerometer
  accel_service_set_sampling_rate	(ACCEL_SAMPLING_25HZ);
  accel_data_service_subscribe(NUM_SAMPLES, (AccelDataHandler) accel_new_data);
  
  app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);

	int in_size = app_message_inbox_size_maximum();
	int out_size = app_message_outbox_size_maximum();
	app_log(APP_LOG_LEVEL_INFO, "C", 0, "I/O Buffer: %d/%d", in_size, out_size);
	app_message_open(in_size, out_size);

	set_text(title,"SWIM\nTAG",FONT_KEY_BITHAM_42_BOLD );
  
  
  
  /*tp = fdevopen("test.txt", "w");
  if(tp == NULL){
  set_text(status,"File Not Open",FONT_KEY_GOTHIC_24_BOLD);
  }*/
  
	// Push the window
	window_stack_push(window, true);
	
	// App Logging!
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Just pushed a window!");
}


void handle_deinit(void) {
  // Unsubscribe from the accelerometer service
  //data_logging_finish(logging_sesh);
  accel_data_service_unsubscribe();
  
	// Destroy the text layers
	text_layer_destroy(title);
  text_layer_destroy(status);
  
  free(memory);
	
	// Destroy the window
	window_destroy(window);
}

int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
}
