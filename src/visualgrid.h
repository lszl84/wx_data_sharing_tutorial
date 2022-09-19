#include <vector>
#include <wx/wx.h>

class VisualGrid : public wxWindow
{
public:
    VisualGrid(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, int xSquaresCount, std::vector<float> &v);

private:
    void OnPaint(wxPaintEvent &evt);

    int xSquaresCount;

    std::vector<float> &values;
};