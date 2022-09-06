#include <wx/wx.h>
#include <chrono>
#include <atomic>
#include <thread>

class MyApp : public wxApp
{
public:
    virtual bool OnInit();
};

class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size);
    virtual ~MyFrame() {}

private:
    wxButton *button;
    wxButton *otherButton;

    wxStaticText *label;
    wxGauge *progressBar;

    bool processing{false};
    std::atomic<bool> quitRequested{false};

    std::thread backgroundThread;

    void OnButtonClick(wxCommandEvent &e);
    void OnClose(wxCloseEvent &e);
};

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    MyFrame *frame = new MyFrame("Hello World", wxPoint(150, 150), wxDefaultSize);
    frame->Show(true);
    return true;
}

MyFrame::MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size)
    : wxFrame(NULL, wxID_ANY, title, pos, size)
{
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    wxSizer *centeringSizer = new wxBoxSizer(wxHORIZONTAL);

    wxPanel *panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, this->FromDIP(wxSize(320, 240)));
    wxSizer *panelSizer = new wxBoxSizer(wxVERTICAL);

    label = new wxStaticText(panel, wxID_ANY, "Click Start", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    progressBar = new wxGauge(panel, wxID_ANY, 1000);

    button = new wxButton(panel, wxID_ANY, "Start");
    button->Bind(wxEVT_BUTTON, &MyFrame::OnButtonClick, this);

    otherButton = new wxButton(panel, wxID_ANY, "Dummy Button");

    panelSizer->Add(label, 1, wxEXPAND | wxALL, this->FromDIP(20));
    panelSizer->Add(progressBar, 1, wxEXPAND | wxALL, this->FromDIP(20));
    panelSizer->Add(button, 0, wxALIGN_CENTER | wxALL, this->FromDIP(5));
    panelSizer->Add(otherButton, 0, wxALIGN_CENTER | wxBOTTOM, this->FromDIP(20));

    panel->SetSizer(panelSizer);

    centeringSizer->Add(panel, 0, wxALIGN_CENTER);
    sizer->Add(centeringSizer, 1, wxALIGN_CENTER);

    this->SetSizerAndFit(sizer);

    this->Bind(wxEVT_CLOSE_WINDOW, &MyFrame::OnClose, this);
}

void MyFrame::OnButtonClick(wxCommandEvent &e)
{
    if (!this->processing)
    {
        this->processing = true;

        std::cout << "1. Current thread " << std::this_thread::get_id() << std::endl;

        const auto f = [this]()
        {
            std::cout << "2. Current thread " << std::this_thread::get_id() << std::endl;

            std::vector<int> arr(50000, 5);
            arr.back() = 3;

            int n = arr.size();
            wxGetApp().CallAfter([this, n]()
                                 {
                                     this->label->SetLabelText(wxString::Format("Sorting the array of %d elements...", n));
                                     this->Layout(); });

            auto start = std::chrono::steady_clock::now();
            for (int i = 0; i < n - 1; i++)
            {
                wxGetApp().CallAfter([this, n, i]()
                                     { this->progressBar->SetValue(i * this->progressBar->GetRange() / (n - 2)); });

                if (this->quitRequested)
                {
                    wxGetApp().CallAfter([this]()
                                         {
                                             this->backgroundThread.join();
                                             this->processing = false;
                                             this->quitRequested = false;
                                             this->Destroy(); });
                    return;
                }

                for (int j = 0; j < n - i - 1; j++)
                {
                    if (arr[j] > arr[j + 1])
                    {
                        std::swap(arr[j], arr[j + 1]);
                    }
                }
            }

            auto end = std::chrono::steady_clock::now();
            auto diff = end - start;

            auto frontValue = arr.front();
            wxGetApp().CallAfter([this, diff, frontValue]()
                                 {
                                     this->label->SetLabelText(wxString::Format("The first number is: %d.\nProcessing time: %.2f [ms]", frontValue, std::chrono::duration<double, std::milli>(diff).count()));
                                     this->Layout();

                                     this->backgroundThread.join();
                                     this->processing = false; });
        };

        this->backgroundThread = std::thread{f};
    }
}

void MyFrame::OnClose(wxCloseEvent &e)
{
    if (this->processing)
    {
        e.Veto();

        this->quitRequested = true;
    }
    else
    {
        this->Destroy();
    }
}