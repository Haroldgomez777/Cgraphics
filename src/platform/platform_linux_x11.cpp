#include "core/context.hpp"
#include "core/window_impl.hpp"
#include "render/gl_surface.hpp"

#include <GL/gl.h>
#include <GL/glx.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <unordered_map>

namespace cgfx::x11_detail {

static float ApproxDpiScale(Display *dpy, int screen) noexcept {
  const int hpx = DisplayHeight(dpy, screen);
  const int hmm = DisplayHeightMM(dpy, screen);
  if (hpx <= 0 || hmm <= 0) {
    return 1.0f;
  }
  const float dpi =
      (static_cast<float>(hpx) * 25.4f) / static_cast<float>(hmm);
  return (dpi > 1.0e-6f) ? (dpi / 96.0f) : 1.0f;
}

static cgfx_mouse_button MapButton(unsigned int btn) noexcept {
  switch (btn) {
  case Button1:
    return CGFX_MOUSE_LEFT;
  case Button2:
    return CGFX_MOUSE_MIDDLE;
  case Button3:
    return CGFX_MOUSE_RIGHT;
  case Button4:
    return CGFX_MOUSE_X1;
  default:
    return CGFX_MOUSE_X2;
  }
}

static cgfx_key MapKeysym(KeySym sym) noexcept {
  switch (sym) {
  case XK_Escape:
    return CGFX_KEY_ESCAPE;
  case XK_space:
    return CGFX_KEY_SPACE;
  case XK_Return:
    return CGFX_KEY_ENTER;
  case XK_Tab:
    return CGFX_KEY_TAB;
  case XK_BackSpace:
    return CGFX_KEY_BACKSPACE;
  case XK_Delete:
    return CGFX_KEY_DELETE;
  case XK_Left:
    return CGFX_KEY_LEFT;
  case XK_Right:
    return CGFX_KEY_RIGHT;
  case XK_Up:
    return CGFX_KEY_UP;
  case XK_Down:
    return CGFX_KEY_DOWN;
  case XK_Home:
    return CGFX_KEY_HOME;
  case XK_End:
    return CGFX_KEY_END;
  case XK_Page_Up:
    return CGFX_KEY_PAGE_UP;
  case XK_Page_Down:
    return CGFX_KEY_PAGE_DOWN;
  case XK_F1:
    return CGFX_KEY_F1;
  case XK_F2:
    return CGFX_KEY_F2;
  case XK_F3:
    return CGFX_KEY_F3;
  case XK_F4:
    return CGFX_KEY_F4;
  case XK_F5:
    return CGFX_KEY_F5;
  case XK_F6:
    return CGFX_KEY_F6;
  case XK_F7:
    return CGFX_KEY_F7;
  case XK_F8:
    return CGFX_KEY_F8;
  case XK_F9:
    return CGFX_KEY_F9;
  case XK_F10:
    return CGFX_KEY_F10;
  case XK_F11:
    return CGFX_KEY_F11;
  case XK_F12:
    return CGFX_KEY_F12;
  case XK_Shift_L:
    return CGFX_KEY_LEFT_SHIFT;
  case XK_Shift_R:
    return CGFX_KEY_RIGHT_SHIFT;
  case XK_Control_L:
    return CGFX_KEY_LEFT_CONTROL;
  case XK_Control_R:
    return CGFX_KEY_RIGHT_CONTROL;
  case XK_Alt_L:
    return CGFX_KEY_LEFT_ALT;
  case XK_Alt_R:
    return CGFX_KEY_RIGHT_ALT;
  default:
    break;
  }

  KeySym upper = sym;
  if ((upper >= XK_a) && (upper <= XK_z)) {
    upper = upper - XK_a + XK_A;
  }

  if ((upper >= XK_A) && (upper <= XK_Z)) {
    const int delta = static_cast<int>(upper - XK_A);
    return static_cast<cgfx_key>(static_cast<int>(CGFX_KEY_A) + delta);
  }

  return CGFX_KEY_UNKNOWN;
}

class X11Surface;

class X11Backend final : public PlatformBackend {
public:
  X11Backend()
      : dpy_(XOpenDisplay(nullptr)),
        screen_(dpy_ ? DefaultScreen(dpy_) : 0),
        wm_protocols_(dpy_ ? XInternAtom(dpy_, "WM_PROTOCOLS", False)
                            : None),
        wm_delete_window_(dpy_ ? XInternAtom(dpy_, "WM_DELETE_WINDOW", False)
                               : None) {}

  ~X11Backend() override {
    by_window_.clear();
    if (dpy_) {
      XCloseDisplay(dpy_);
      dpy_ = nullptr;
    }
  }

  X11Backend(const X11Backend &) = delete;
  X11Backend &operator=(const X11Backend &) = delete;

  Display *display() const noexcept { return dpy_; }
  Atom protocols_atom() const noexcept { return wm_protocols_; }
  Atom delete_atom() const noexcept { return wm_delete_window_; }

  void register_window(::Window w, X11Surface *surf) noexcept {
    if ((w != 0U) && surf) {
      by_window_[w] = surf;
    }
  }

  void unregister_window(::Window w) noexcept { by_window_.erase(w); }

  X11Surface *surface_for(::Window w) noexcept {
    auto it = by_window_.find(w);
    return (it == by_window_.end()) ? nullptr : it->second;
  }

  void poll_events(CgfxContext *ctx) override;

  std::unique_ptr<PlatformSurface>
  create_surface(CgfxWindow *owner, const cgfx_window_desc *desc,
                 cgfx_result &status) override;

private:
  Display *dpy_{nullptr};
  int screen_{0};
  Atom wm_protocols_{None};
  Atom wm_delete_window_{None};
  std::unordered_map<::Window, X11Surface *> by_window_{};
};

class X11Surface final : public PlatformSurface {
public:
  explicit X11Surface(CgfxWindow *owner, X11Backend *backend) noexcept
      : PlatformSurface(owner), backend_(backend) {}

  X11Surface(const X11Surface &) = delete;
  X11Surface &operator=(const X11Surface &) = delete;

  ~X11Surface() override { teardown(); }

  cgfx_result initialize(const cgfx_window_desc *desc);
  void teardown() noexcept override;

  void dispatch_event(const XEvent &evt) noexcept;

  cgfx_result begin_present(uint32_t *out_width, uint32_t *out_height,
                            float *out_scale) override;

  void end_present() override;

  cgfx_result clear_normalized_rgba(float r, float g, float b,
                                    float a) override;

  cgfx_result query_size_px(uint32_t *out_w, uint32_t *out_h) const override;

  cgfx_result query_dpi_scale(float *out_scale) const override;

private:
  void refresh_geometry_from_server() noexcept;
  void enqueue_resize_event() noexcept;

private:
  X11Backend *backend_{nullptr};
  Display *dpy_{nullptr};
  int screen_{0};

  ::Window window_{0};
  GLXContext glx_{nullptr};
  bool owns_{false};
  bool presenting_{false};

  long client_w_{1};
  long client_h_{1};
};

void X11Surface::refresh_geometry_from_server() noexcept {
  if ((!dpy_) || (window_ == 0U)) {
    return;
  }

  XWindowAttributes attr{};
  if (XGetWindowAttributes(dpy_, window_, &attr) == 0) {
    return;
  }

  client_w_ = std::max<long>(static_cast<long>(attr.width), 1L);
  client_h_ = std::max<long>(static_cast<long>(attr.height), 1L);
}

void X11Surface::enqueue_resize_event() noexcept {
  QueuedEvent ev{};
  ev.type = CGFX_EVENT_RESIZE;
  ev.window = owner();
  ev.resize.width =
      static_cast<uint32_t>((client_w_ < 1) ? 1 : client_w_);
  ev.resize.height =
      static_cast<uint32_t>((client_h_ < 1) ? 1 : client_h_);
  owner()->context().push_event(ev);
}

cgfx_result X11Surface::initialize(const cgfx_window_desc *desc) {
  if ((!desc) || (!backend_)) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  dpy_ = backend_->display();
  if (!dpy_) {
    return CGFX_ERROR_PLATFORM;
  }

  screen_ = DefaultScreen(dpy_);

  static int attrib[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 24,
                         None};
  XVisualInfo *vi = glXChooseVisual(dpy_, screen_, attrib);
  if (!vi) {
    return CGFX_ERROR_PLATFORM;
  }

  ::Window root = RootWindow(dpy_, screen_);

  Colormap cmap = XCreateColormap(dpy_, root, vi->visual, AllocNone);

  XSetWindowAttributes swa{};
  swa.colormap = cmap;
  swa.border_pixel = BlackPixel(dpy_, screen_);
  swa.background_pixel = WhitePixel(dpy_, screen_);
  swa.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask |
                   KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
                   PointerMotionMask;

  const unsigned long vm =
      CWColormap | CWBorderPixel | CWBackPixel | CWEventMask;

  const unsigned int ww =
      desc->width > 0 ? static_cast<unsigned int>(desc->width) : 960U;

  const unsigned int hh =
      desc->height > 0 ? static_cast<unsigned int>(desc->height) : 540U;

  window_ = XCreateWindow(dpy_, root, 50, 50, ww, hh, 0, vi->depth,
                          InputOutput, vi->visual, vm, &swa);

  if (window_ == 0U) {
    XFree(vi);
    return CGFX_ERROR_PLATFORM;
  }

  glx_ = glXCreateContext(dpy_, vi, nullptr, GL_TRUE);
  XFree(vi);
  vi = nullptr;

  if (!glx_) {
    XDestroyWindow(dpy_, window_);
    window_ = 0U;
    return CGFX_ERROR_PLATFORM;
  }

  backend_->register_window(window_, this);
  owns_ = true;

  Atom wm_del = backend_->delete_atom();
  if (wm_del != None) {
    XSetWMProtocols(dpy_, window_, &wm_del, 1);
  }

  if (desc->title) {
    XStoreName(dpy_, window_, desc->title);
  } else {
    XStoreName(dpy_, window_, "cgfx");
  }

  XMapWindow(dpy_, window_);
  XFlush(dpy_);

  if (glXMakeCurrent(dpy_, window_, glx_)) {
    refresh_geometry_from_server();
    enqueue_resize_event();
    gl_raster::viewport_pixels(0, 0, static_cast<int>(client_w_),
                               static_cast<int>(client_h_));
    (void)glXMakeCurrent(dpy_, None, nullptr);
  }

  return CGFX_OK;
}

void X11Surface::teardown() noexcept {
  presenting_ = false;

  if (!owns_) {
    backend_ = nullptr;
    window_ = 0U;
    dpy_ = nullptr;
    return;
  }

  owns_ = false;

  if (dpy_) {
    (void)glXMakeCurrent(dpy_, None, nullptr);
  }

  if ((dpy_) && (glx_)) {
    glXDestroyContext(dpy_, glx_);
    glx_ = nullptr;
  }

  if (backend_ != nullptr && window_ != 0U) {
    backend_->unregister_window(window_);
  }

  backend_ = nullptr;

  if ((dpy_ != nullptr) && (window_ != 0U)) {
    XDestroyWindow(dpy_, window_);
    XFlush(dpy_);
  }

  window_ = 0U;
  dpy_ = nullptr;
}

void X11Surface::dispatch_event(const XEvent &evt) noexcept {
  switch (evt.type) {
  case ConfigureNotify:
    refresh_geometry_from_server();
    enqueue_resize_event();
    return;

  case ClientMessage:
    if (evt.xclient.format == 32 &&
        evt.xclient.message_type == backend_->protocols_atom() &&
        static_cast<Atom>(evt.xclient.data.l[0]) == backend_->delete_atom()) {

      QueuedEvent ev{};
      ev.type = CGFX_EVENT_CLOSE_REQUEST;
      ev.window = owner();
      ev.close.unused = 0;
      owner()->context().push_event(ev);
    }
    return;

  case MotionNotify: {
    QueuedEvent ev{};
    ev.type = CGFX_EVENT_MOUSE_MOVE;
    ev.window = owner();
    ev.move.x = evt.xmotion.x;
    ev.move.y = evt.xmotion.y;
    owner()->context().push_event(ev);
    return;
  }

  case ButtonPress:
  case ButtonRelease: {
    QueuedEvent ev{};
    ev.type = CGFX_EVENT_MOUSE_BUTTON;
    ev.window = owner();
    ev.mb.button = MapButton(static_cast<unsigned int>(evt.xbutton.button));
    ev.mb.action =
        evt.type == ButtonPress ? CGFX_PRESS : CGFX_RELEASE;
    ev.mb.x = evt.xbutton.x;
    ev.mb.y = evt.xbutton.y;
    owner()->context().push_event(ev);
    return;
  }

  case KeyPress:
  case KeyRelease: {

    QueuedEvent ev{};
    ev.type = CGFX_EVENT_KEY;
    ev.window = owner();
    KeySym ks = XLookupKeysym(&(const_cast<XEvent &>(evt)).xkey, 0);
    ev.key.key = MapKeysym(ks);
    ev.key.native_code = static_cast<uint32_t>(ks);
    ev.key.action =
        evt.type == KeyPress ? CGFX_PRESS : CGFX_RELEASE;
    ev.key.repeat = 0;
    owner()->context().push_event(ev);
    return;
  }

  default:
    return;
  }
}

cgfx_result X11Surface::begin_present(uint32_t *out_width,
                                     uint32_t *out_height,
                                     float *out_scale) {

  if ((!dpy_) || (window_ == 0U) || (!glx_)) {
    return CGFX_ERROR_PLATFORM;
  }
  refresh_geometry_from_server();

  if (glXMakeCurrent(dpy_, window_, glx_) == GL_FALSE) {
    return CGFX_ERROR_PLATFORM;
  }


  const int vw = static_cast<int>(client_w_);
  const int vh = static_cast<int>(client_h_);
  gl_raster::viewport_pixels(0, 0,
                             vw > 1 ? vw : 1,
                             vh > 1 ? vh : 1);

  presenting_ = true;

  const float s = ApproxDpiScale(dpy_, screen_);

  if (out_width) {
    *out_width = static_cast<uint32_t>(client_w_);
  }

  if (out_height) {
    *out_height = static_cast<uint32_t>(client_h_);
  }

  if (out_scale) {
    *out_scale = s;
  }

  return CGFX_OK;
}

















void















    X11Surface::end_present()





{















  if ((!dpy_)



          ||

      (window_




              ==




              0)



          ||

      (!glx_))



  {

















    presenting_ =




        false;



    return;



  }

















  if (presenting_)



  {

















    glXSwapBuffers




        (dpy_, window_);



  }



















  presenting_




      =




      false;



  (void)



      glXMakeCurrent




          (




              dpy_,






              None,





              nullptr);



}























cgfx_result X11Surface::




    clear_normalized_rgba(float r,




                              float




                                  g,




                          float




                              b,




                          float




                              a)





{




  if ((!presenting_)



          ||

      (!



          dpy_)



          ||

      (




          window_




              ==




              0)



          ||

      (!




          glx_))



  {




    return CGFX_ERROR_PLATFORM;




  }

















  if (glXMakeCurrent(dpy_, window_, glx_) == GL_FALSE)




  {















    return CGFX_ERROR_PLATFORM;




  }

















  return gl_raster::clear_color_buffer_normalized(r,




                                                           g,




                                                  b,




                                                  a);




}






cgfx_result X11Surface::query_size_px(uint32_t *out_w,



                                                     uint32_t *



                                                                     out_h)





                                        const















{













  if ((!out_w)



          ||

      (!




          out_h))



  {




    return




        CGFX_ERROR_INVALID_ARGUMENT;



  }

















  X11Surface *

      mut =






          const_cast<X11Surface *>



              (




                  this);



  mut->















      refresh_geometry_from_server















          ();


















  *

      out_w = static_cast<uint32_t>((mut->















                                      client_




                                                  w















                                          <















                                              1)




                                                                  ?















                                              1
                                          :















                                              mut->











client_w_);















 *out_h =















         static_cast<uint32_t>((mut















                                   ->






                                   client_




                                                               h















                                   <















                                       1)




                                                           ? 1















                                       :















                                           mut















                                               ->















                                               client_));













 return CGFX_OK;




}







cgfx_result X11Surface::query_dpi_scale(float *















                                         out)



                                            const















{















  if ((!out)



          ||

      (!



          dpy_))



  {




    return CGFX_ERROR_INVALID_ARGUMENT;



  }



  *out = ApproxDpiScale




          (




              dpy_, screen_);



 return CGFX_OK;



}

















void















    X11Backend::poll_events(CgfxContext *ctx)



{













  (void)



      ctx;













  if ((!dpy_)



          ||

      !display())



  {




    return;



  }

















  while (XPending




             (




                 dpy_)



         >



             0)



  {

















    XEvent ev{};


















    XNextEvent




        (




            dpy_,






            &






                ev);

















    X11Surface *

        surf







            =















                surface_for







                    (ev.















                         xany.













                         window















                    );


















    if (!surf)



    {

















      continue;



    }

















    surf















        ->















        dispatch_event







            (















                ev















            );





  }





}

















std




    ::unique_ptr




        <




            PlatformSurface>

















            X11Backend::















                create_surface(CgfxWindow *owner,




                                               const




                                                   cgfx_window_desc *




                                                       desc,




                                               cgfx_result &




                                                            status)



{
















  status















      =















      CGFX_OK;


















  if ((!owner)



          ||

      (!




          desc)



          ||

      (!display()))



  {




    status = CGFX_ERROR_INVALID_ARGUMENT;







    if (!




        display())



    {

















      status = CGFX_ERROR_PLATFORM;



    }

















    return




        nullptr;



  }


















  auto surf = std::make_unique<X11Surface>(owner,




                                                                     this);



  status















      =















      surf















          ->initialize(desc);


















  if (status















      !=















      CGFX_OK)



  {

















    surf















        ->















        teardown















            ();


















    surf.







        reset















            ();


















    return nullptr;



  }


















  return surf;



}






}




namespace cgfx {







std















    ::















    unique_ptr<PlatformBackend>



        cgfx_make_x11_backend()



{















  auto bk =















      std















          ::






              make_unique<x11_detail::




                              X11Backend>();

















  if ((!bk)



          ||

      (!bk



              ->















              display()))



  {




    return nullptr;



  }



  return bk;



}




}



