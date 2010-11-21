/*
  Zajal - Creative coding platform 
  Copyright (C) 2010 Ramsey Nasser
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// interpreter class

#include <sys/stat.h>

#include "ruby.h"
#include "ZajalInterpreter.h"

#include "graphics.h"
#include "events.h"

// for procs with no arguments, no need for specialized safe function
VALUE zj_safe_proc_call(VALUE proc) {
  if(!NIL_P(proc)) rb_funcall(proc, rb_intern("call"), 0);
}

VALUE zj_safe_mouse_moved_call(VALUE args) {
  VALUE mouse_moved_proc = ((VALUE*)args)[0];
  VALUE x = ((VALUE*)args)[1];
  VALUE y = ((VALUE*)args)[2];
  
  if(!NIL_P(mouse_moved_proc)) rb_funcall(mouse_moved_proc, rb_intern("call"), 2, x, y);
}

VALUE zj_safe_mouse_dragged_call(VALUE args) {
  VALUE mouse_dragged_proc = ((VALUE*)args)[0];
  VALUE x = ((VALUE*)args)[1];
  VALUE y = ((VALUE*)args)[2];
  VALUE button = ((VALUE*)args)[3];
  
  if(!NIL_P(mouse_dragged_proc)) rb_funcall(mouse_dragged_proc, rb_intern("call"), 3, x, y, button);
}

VALUE zj_safe_mouse_pressed_call(VALUE args) {
  VALUE mouse_pressed_proc = ((VALUE*)args)[0];
  VALUE x = ((VALUE*)args)[1];
  VALUE y = ((VALUE*)args)[2];
  VALUE button = ((VALUE*)args)[3];
  
  if(!NIL_P(mouse_pressed_proc)) rb_funcall(mouse_pressed_proc, rb_intern("call"), 3, x, y, button);
}

VALUE zj_safe_mouse_released_call(VALUE args) {
  VALUE mouse_released_proc = ((VALUE*)args)[0];
  VALUE x = ((VALUE*)args)[1];
  VALUE y = ((VALUE*)args)[2];
  VALUE button = ((VALUE*)args)[3];
  
  if(!NIL_P(mouse_released_proc)) rb_funcall(mouse_released_proc, rb_intern("call"), 3, x, y, button);
}

VALUE zj_safe_key_pressed_call(VALUE args) {
  VALUE key_pressed_proc = ((VALUE*)args)[0];
  VALUE key = ((VALUE*)args)[1];
  if(!NIL_P(key_pressed_proc)) rb_funcall(key_pressed_proc, rb_intern("call"), 1, key);
}

VALUE zj_safe_key_released_call(VALUE args) {
  VALUE key_released_proc = ((VALUE*)args)[0];
  VALUE key = ((VALUE*)args)[1];
  if(!NIL_P(key_released_proc)) rb_funcall(key_released_proc, rb_intern("call"), 1, key);
}

VALUE zj_safe_window_resized_call(VALUE args) {
  VALUE window_resized_proc = ((VALUE*)args)[0];
  VALUE w = ((VALUE*)args)[1];
  VALUE h = ((VALUE*)args)[2];
  
  if(!NIL_P(window_resized_proc)) rb_funcall(window_resized_proc, rb_intern("call"), 2, w, h);
}

VALUE zj_safe_load_new_script(VALUE code) {
  VALUE newContext = rb_funcall(zj_cContext, rb_intern("new"), 0);
  rb_funcall(newContext, rb_intern("instance_eval"), 1, code);
  return newContext;
}

VALUE zj_button_to_symbol(int button) {
  if(button == 0)
    return ID2SYM(rb_intern("left"));
  else if(button == 1)
    return ID2SYM(rb_intern("center"));
  else if(button == 2)
    return ID2SYM(rb_intern("right"));
  else
    rb_bug("Received unsupported button `%d'on mouseDragged! Bailing out!", button);
}

ZajalInterpreter::ZajalInterpreter(char* fileName) {
  scriptName = (char*)malloc(SCRIPT_NAME_SIZE*sizeof(char));
  
  strncpy(scriptName, fileName, SCRIPT_NAME_SIZE);
  strncat(scriptName, "\0", SCRIPT_NAME_SIZE);
  
  scriptModifiedTime = 0;
  
  nextUpdate = SCRIPT_UPDATE_FREQUENCY;
  lastErrorMessage = (char*)malloc(ERROR_MESSAGE_SIZE*sizeof(char));
  currentContext = Qnil;
}

//--------------------------------------------------------------
void ZajalInterpreter::setup() {
  // defaults
  ofSetFrameRate(30);
  ofDisableSmoothing();
  ofDisableAlphaBlending();
  ofSetColor(240, 240, 240, 255);
  ofBackground(32, 32, 32);
  ofSetBackgroundAuto(true);
  ofSetCircleResolution(32);
  ofSetRectMode(OF_RECTMODE_CORNER);
  ofSetLineWidth(1.0);
  
  if(!lastError && !NIL_P(currentContext)) {
    // if no error exists, run user setup method, catch runtime errors
    VALUE setup_proc = rb_iv_get(currentContext, "setup_proc");
    rb_protect(zj_safe_proc_call, setup_proc, &lastError);
    handleError(lastError);
  }
}

//--------------------------------------------------------------
void ZajalInterpreter::update() {
  if(!lastError && !NIL_P(currentContext)) {
    // if no error exists, run user update method, catch runtime errors
    VALUE update_proc = rb_iv_get(currentContext, "update_proc");
    rb_protect(zj_safe_proc_call, update_proc, &lastError);
    handleError(lastError);
  }
}

void ZajalInterpreter::updateCurrentScript() {
  struct stat attrib;
  if(stat(scriptName, &attrib)) {
    printf("Could not stat %s!\n", scriptName);
    
  } else {
    if(attrib.st_mtimespec.tv_sec > scriptModifiedTime) {
      printf("Updating %s in place...\n", scriptName);
      scriptModifiedTime = attrib.st_mtimespec.tv_sec;
      loadScript(scriptName);
      
    }
    
  }
  
  nextUpdate = SCRIPT_UPDATE_FREQUENCY;
}

//--------------------------------------------------------------
void ZajalInterpreter::draw() {
  if(lastError && !NIL_P(currentContext)) {
    // an error exists, draw error screen
    ofSetColor(255, 255, 255, 255);
    lastErrorImage.draw(0, 0);
    // TODO apply filters to lastErrorImage instead of drawing a rect
    ofEnableAlphaBlending();
    ofSetColor(255, 255, 255, 128);
    ofRect(0, 0, ofGetWidth(), ofGetHeight());
    ofSetColor(255, 255, 255, 255);
    ofRect(0, ofGetHeight()/2-50, ofGetWidth(), 100);
    ofSetColor(0, 0, 0, 255);
    ofDrawBitmapString(lastErrorMessage, 10, ofGetHeight()/2-30);
    
  } else {
    // no error exists, draw next frame of user code, catch runtime errors
    zj_graphics_reset_frame();
    VALUE draw_proc = rb_iv_get(currentContext, "draw_proc");
    rb_protect(zj_safe_proc_call, draw_proc, &lastError);
    handleError(lastError);
    
  }
  
  // try to update script at end of setup-update-draw loop
  if(nextUpdate-- == 0) updateCurrentScript();
}


//--------------------------------------------------------------
void ZajalInterpreter::keyPressed  (int key) {
  if(!lastError && !NIL_P(currentContext)) {
    // TODO convert key into symbols
    VALUE args[2];
    args[0] = rb_iv_get(currentContext, "key_pressed_proc");
    args[1] = INT2FIX(key);
    
    rb_protect(zj_safe_key_pressed_call, (VALUE)args, &lastError);
    handleError(lastError);
  }
}

//--------------------------------------------------------------
void ZajalInterpreter::keyReleased  (int key) {
  if(!lastError && !NIL_P(currentContext)) {
    // TODO convert key into symbols
    VALUE args[2];
    args[0] = rb_iv_get(currentContext, "key_released_proc");
    args[1] = INT2FIX(key);
    
    rb_protect(zj_safe_key_released_call, (VALUE)args, &lastError);
    handleError(lastError);
  }
}

//--------------------------------------------------------------
// http://www.ruby-forum.com/topic/76498
void ZajalInterpreter::mouseMoved(int x, int y) {
  if(!lastError && !NIL_P(currentContext)) {
    VALUE args[3];
    args[0] = rb_iv_get(currentContext, "mouse_moved_proc");
    args[1] = INT2FIX(x);
    args[2] = INT2FIX(y);
    
    rb_protect(zj_safe_mouse_moved_call, (VALUE)args, &lastError);
    handleError(lastError);
  }
}

//--------------------------------------------------------------
void ZajalInterpreter::mouseDragged(int x, int y, int button) {
  if(!lastError && !NIL_P(currentContext)) {
    VALUE args[4];
    args[0] = rb_iv_get(currentContext, "mouse_dragged_proc");
    args[1] = INT2FIX(x);
    args[2] = INT2FIX(y);
    args[3] = zj_button_to_symbol(button);
    
    rb_protect(zj_safe_mouse_dragged_call, (VALUE)args, &lastError);
    handleError(lastError);
  }
}

//--------------------------------------------------------------
void ZajalInterpreter::mousePressed(int x, int y, int button) {
  if(!lastError && !NIL_P(currentContext)) {
    VALUE args[4];
    args[0] = rb_iv_get(currentContext, "mouse_pressed_proc");
    args[1] = INT2FIX(x);
    args[2] = INT2FIX(y);
    args[3] = zj_button_to_symbol(button);
    
    rb_protect(zj_safe_mouse_pressed_call, (VALUE)args, &lastError);
    handleError(lastError);
  }
}


//--------------------------------------------------------------
void ZajalInterpreter::mouseReleased(int x, int y, int button) {
  if(!lastError && !NIL_P(currentContext)) {
    VALUE args[3];
    args[0] = rb_iv_get(currentContext, "mouse_released_proc");
    args[1] = INT2FIX(x);
    args[2] = INT2FIX(y);
    args[3] = zj_button_to_symbol(button);
    
    rb_protect(zj_safe_mouse_released_call, (VALUE)args, &lastError);
    handleError(lastError);
  }
}

//--------------------------------------------------------------
void ZajalInterpreter::windowResized(int w, int h) {
  if(!lastError && !NIL_P(currentContext)) {
    VALUE args[3];
    args[0] = rb_iv_get(currentContext, "window_resized_proc");
    args[1] = INT2FIX(w);
    args[2] = INT2FIX(h);
    
    rb_protect(zj_safe_window_resized_call, (VALUE)args, &lastError);
    handleError(lastError);
  }
}

void ZajalInterpreter::loadScript(char* filename) {
  scriptName = filename;
  
  // open file, measure size
  FILE *scriptFile = fopen(scriptName, "r");
  fseek(scriptFile, 0, SEEK_END);
  long scriptFileSize = ftell(scriptFile);
  fseek(scriptFile, 0, SEEK_SET);
  
  printf("Reading %s (%db)\n", scriptName, scriptFileSize);
  
  // load file into memory
  char* scriptFileContent = (char*)malloc(scriptFileSize * sizeof(char) + 1);
  fread(scriptFileContent, scriptFileSize, 1, scriptFile);
  scriptFileContent[scriptFileSize * sizeof(char)] = '\0';
  fclose(scriptFile);
  
  // execute contents of file, catch syntax errors
  ruby_script(scriptName);
  VALUE newContext = rb_protect(zj_safe_load_new_script, rb_str_new2(scriptFileContent), &lastError);
  if(!lastError) {
    if(NIL_P(currentContext)) {
      currentContext = newContext;
      setup();
    } else {
      currentContext = newContext;
      //TODO more inteligent code updating, i.e. just the draw proc
      // currentContext = newContext;
      // printf("currentContext.draw_proc : %s\n", RSTRING_PTR(rb_obj_as_string(rb_iv_get(currentContext, "draw_proc"))) );
      // rb_ivar_set(currentContext, rb_intern("draw_proc"), rb_ivar_get(newContext, rb_intern("draw_proc")));
      // printf("currentContext.draw_proc : %s\n", RSTRING_PTR(rb_obj_as_string(rb_iv_get(currentContext, "draw_proc"))) );
    }
    
  } else {
    handleError(lastError);
    
  }
  
}

// http://metaeditor.sourceforge.net/embed/
void ZajalInterpreter::handleError(int error) {
  if(error) {
    VALUE last_error = rb_gv_get("$!");
    char* error_class = RSTRING_PTR(rb_class_path(CLASS_OF(last_error)));
    char* error_message = RSTRING_PTR(rb_obj_as_string(last_error));
    
    // class
    cout << "class = " << error_class << endl; 

    // message
    cout << "message = " << error_message << endl;
    
    lastErrorImage.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
    
    size_t error_message_size = strlen(error_message);
    memset(lastErrorMessage, 0, ERROR_MESSAGE_SIZE);
    strncpy(lastErrorMessage, error_message, error_message_size);
    strncat(lastErrorMessage, "\n", ERROR_MESSAGE_SIZE);
    
    // backtrace
    if(!NIL_P(last_error)) {
        std::ostringstream o;
        VALUE backtrace = rb_funcall(last_error, rb_intern("backtrace"), 0);
        long backtrace_length = RARRAY_LEN(backtrace);
        VALUE* backtrace_ptr = RARRAY_PTR(backtrace);
        
        if(backtrace_length > 1) strncat(lastErrorMessage, RSTRING_PTR(backtrace_ptr[0]), ERROR_MESSAGE_SIZE - error_message_size);
        for(int c=0; c<backtrace_length; c++) {
            o << "\tfrom " << RSTRING_PTR(backtrace_ptr[c]) << "\n";
        }
        cout << "backtrace = \n" << o.str() << endl;
    }
  }
}
