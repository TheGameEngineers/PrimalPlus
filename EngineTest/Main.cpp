// NOTE: replace the #endif at the end of Main.cpp in Primal Engine Test with the following...

#elif __linux__
#include <X11/Xlib.h>

int main(int argc, char* argv[])
{
	XInitThreads();
    
    engine_test test{};

    // Open an X server connection
    Display* display { XOpenDisplay(NULL) };
        if (display == NULL) return 1;
    
    // Set up custom client messages
    Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", false);
    Atom quit_msg = XInternAtom(display, "QUIT_MSG", false);

	if (test.initialize(display))
	{
        XEvent xev;
        bool is_running{ true };
        while (is_running)
        {
            // NOTE: we use an if statement here because we are not handling all events in this translation
            //       unit, so XPending(display) will often not ever be 0, and therefore this can create
            //       an infinite loop... but this protects XNextEvent from blocking if there are no events.
            if (XPending(display) > 0)
            {
                XNextEvent(display, &xev);

                switch (xev.type)
                {
                    case KeyPress:
                        
                        break;
                    case ClientMessage:
                        if ((Atom)xev.xclient.data.l[0] == wm_delete_window)
                        {
                            // Dont handle this here
                            XPutBackEvent(display, &xev);
                        }
                        if ((Atom)xev.xclient.data.l[0] == quit_msg)
                        {
                            is_running = false;
                        }
                        break;
                }
            }
            test.run(display);
        }
        test.shutdown();
        XCloseDisplay(display);
        return 0;
	}
}
#endif // _WIN64