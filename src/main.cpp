#include <wx/wx.h>
#include <chrono>
#include <atomic>
#include <thread>

#include <random>

#include "visualgrid.h"

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

    wxGauge *progressBar;
    VisualGrid *grid;

    bool processing{false};
    std::atomic<bool> quitRequested{false};

    std::vector<float> sharedData;

    std::thread backgroundThread;

    void OnButtonClick(wxCommandEvent &e);
    void OnClose(wxCloseEvent &e);

    void BackgroundTask();
    void RandomizeSharedData();
};

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    MyFrame *frame = new MyFrame("Hello World", wxPoint(300, 95), wxDefaultSize);
    frame->Show(true);
    return true;
}

MyFrame::MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size)
    : wxFrame(NULL, wxID_ANY, title, pos, size), sharedData(50000)
{
    RandomizeSharedData();
    this->CreateStatusBar(1);

    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    wxPanel *panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    wxSizer *panelSizer = new wxBoxSizer(wxHORIZONTAL);

    button = new wxButton(panel, wxID_ANY, "Start");
    button->Bind(wxEVT_BUTTON, &MyFrame::OnButtonClick, this);

    progressBar = new wxGauge(panel, wxID_ANY, 1000, wxDefaultPosition, this->FromDIP(wxSize(320, 20)));

    panelSizer->Add(button, 0, wxALIGN_CENTER | wxRIGHT, this->FromDIP(5));
    panelSizer->Add(progressBar, 0, wxALIGN_CENTER);

    grid = new VisualGrid(this, wxID_ANY, wxDefaultPosition, this->FromDIP(wxSize(1000, 800)), 250, sharedData);

    panel->SetSizer(panelSizer);

    sizer->Add(panel, 0, wxEXPAND | wxALL, this->FromDIP(5));
    sizer->Add(grid, 0, wxCENTER | wxLEFT | wxRIGHT | wxBOTTOM, this->FromDIP(5));

    this->SetSizerAndFit(sizer);

    this->Bind(wxEVT_CLOSE_WINDOW, &MyFrame::OnClose, this);

    this->SetStatusText("Click Start.", 0);
}

void MyFrame::OnButtonClick(wxCommandEvent &e)
{
    if (!this->processing)
    {
        this->processing = true;

        std::cout << "1. Current thread " << std::this_thread::get_id() << std::endl;

        this->backgroundThread = std::thread{&MyFrame::BackgroundTask, this};
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

void MyFrame::RandomizeSharedData()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distr(0, 1);

    for (int i = 0; i < sharedData.size(); i++)
    {
        sharedData[i] = distr(gen);
    }
}

void MyFrame::BackgroundTask()
{
    std::cout << "2. Current thread " << std::this_thread::get_id() << std::endl;

    int n = sharedData.size();
    wxGetApp().CallAfter([this, n]()
                         {
                                     this->SetStatusText(wxString::Format("Sorting the array of %d elements...", n));
                                     this->Layout(); });

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < n - 1; i++)
    {
        wxGetApp().CallAfter([this, n, i]()
                             { this->progressBar->SetValue(i * this->progressBar->GetRange() / (n - 2)); 
                             this->grid->Refresh(); });

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
            if (sharedData[j] > sharedData[j + 1])
            {
                std::swap(sharedData[j], sharedData[j + 1]);
            }
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;

    auto frontValue = sharedData.front();
    wxGetApp().CallAfter([this, diff, frontValue]()
                         {
                                     this->SetStatusText(wxString::Format("The first number is: %f.\nProcessing time: %.2f [ms]", frontValue, std::chrono::duration<double, std::milli>(diff).count()));
                                     this->Layout();

                                     this->backgroundThread.join();
                                     this->processing = false; });
}