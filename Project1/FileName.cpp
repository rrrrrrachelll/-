#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/artprov.h>
#include <wx/toolbar.h>
#include <wx/treectrl.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/dcbuffer.h>
#include <stack>
#include <fstream>

// ===== 绘图区类 =====
class MyDrawPanel : public wxPanel {
public:
    MyDrawPanel(wxWindow* parent)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE),
        m_zoom(1.0), copiedShape("")
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &MyDrawPanel::OnPaint, this);
    }

    void AddShape(const wxString& shape) {
        m_shapes.push_back(shape);
        undoStack.push(shape);
        while (!redoStack.empty()) redoStack.pop();
        Refresh();
    }

    void Undo() {
        if (!m_shapes.empty()) {
            wxString last = m_shapes.back();
            m_shapes.pop_back();
            redoStack.push(last);
            Refresh();
        }
    }

    void Redo() {
        if (!redoStack.empty()) {
            wxString redoShape = redoStack.top();
            redoStack.pop();
            m_shapes.push_back(redoShape);
            undoStack.push(redoShape);
            Refresh();
        }
    }

    void Copy() {
        if (!m_shapes.empty()) {
            copiedShape = m_shapes.back();
        }
    }

    void Paste() {
        if (!copiedShape.IsEmpty()) {
            AddShape(copiedShape);
        }
    }

    void ZoomIn() {
        m_zoom *= 1.2;
        Refresh();
    }

    void ZoomOut() {
        m_zoom /= 1.2;
        Refresh();
    }

    void ClearAll() {
        m_shapes.clear();
        while (!undoStack.empty()) undoStack.pop();
        while (!redoStack.empty()) redoStack.pop();
        Refresh();
    }

    void SaveToFile(const wxString& filename) {
        std::ofstream ofs(filename.ToStdString());
        for (auto& s : m_shapes) {
            ofs << s.ToStdString() << "\n";
        }
    }

    void LoadFromFile(const wxString& filename) {
        std::ifstream ifs(filename.ToStdString());
        std::string line;
        m_shapes.clear();
        while (std::getline(ifs, line)) {
            m_shapes.push_back(line);
        }
        Refresh();
    }

private:
    std::vector<wxString> m_shapes;
    std::stack<wxString> undoStack;
    std::stack<wxString> redoStack;
    wxString copiedShape;
    double m_zoom;

    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();
        dc.SetPen(*wxBLACK_PEN);

        for (size_t i = 0; i < m_shapes.size(); ++i) {
            wxString shape = m_shapes[i];
            int x = (int)(50 + (i % 3) * 150 * m_zoom);
            int y = (int)(50 + (i / 3) * 150 * m_zoom);

            if (shape == "AND") {
                dc.SetBrush(*wxBLUE_BRUSH);
                dc.DrawRectangle(x, y, int(100 * m_zoom), int(80 * m_zoom));
                dc.DrawText("AND", x + 40, y + 35);
            }
            else if (shape == "OR") {
                dc.SetBrush(*wxGREEN_BRUSH);
                dc.DrawEllipse(x, y, int(120 * m_zoom), int(80 * m_zoom));
                dc.DrawText("OR", x + 50, y + 35);
            }
            else if (shape == "NOT") {
                dc.SetBrush(*wxCYAN_BRUSH);
                wxPoint tri[3] = {
                    wxPoint(x, y),
                    wxPoint(x, y + int(80 * m_zoom)),
                    wxPoint(x + int(80 * m_zoom), y + int(40 * m_zoom))
                };
                dc.DrawPolygon(3, tri);
                dc.DrawText("NOT", x + 20, y + 35);
            }
            else if (shape == "LED") {
                dc.SetBrush(*wxRED_BRUSH);
                dc.DrawCircle(x + int(50 * m_zoom), y + int(50 * m_zoom), int(30 * m_zoom));
                dc.DrawText("LED", x + 40, y + 90);
            }
            else {
                dc.DrawText("未知组件", x, y);
            }
        }
    }
};

// ===== 主窗口 =====
class MyFrame : public wxFrame {
public:
    MyFrame(const wxString& title);

private:
    MyDrawPanel* m_drawPanel;
    wxTreeCtrl* m_treeCtrl;
    wxSplitterWindow* m_splitter;

    void OnNew(wxCommandEvent& event);
    void OnOpen(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnSaveAs(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);

    void OnUndo(wxCommandEvent& event);
    void OnRedo(wxCommandEvent& event);
    void OnCopy(wxCommandEvent& event);
    void OnPaste(wxCommandEvent& event);

    void OnZoomin(wxCommandEvent& event);
    void OnZoomout(wxCommandEvent& event);
    void OnToggleStatusBar(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

    void UpdateTitle();

    wxString m_currentFile;
    wxDECLARE_EVENT_TABLE();
};

enum {
    ID_SHOW_STATUSBAR = wxID_HIGHEST + 1
};

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_MENU(wxID_NEW, MyFrame::OnNew)
EVT_MENU(wxID_OPEN, MyFrame::OnOpen)
EVT_MENU(wxID_SAVE, MyFrame::OnSave)
EVT_MENU(wxID_SAVEAS, MyFrame::OnSaveAs)
EVT_MENU(wxID_EXIT, MyFrame::OnQuit)

EVT_MENU(wxID_UNDO, MyFrame::OnUndo)
EVT_MENU(wxID_REDO, MyFrame::OnRedo)
EVT_MENU(wxID_COPY, MyFrame::OnCopy)
EVT_MENU(wxID_PASTE, MyFrame::OnPaste)

EVT_MENU(wxID_ZOOM_IN, MyFrame::OnZoomin)
EVT_MENU(wxID_ZOOM_OUT, MyFrame::OnZoomout)
EVT_MENU(ID_SHOW_STATUSBAR, MyFrame::OnToggleStatusBar)

EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
wxEND_EVENT_TABLE()

MyFrame::MyFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(1000, 650))
{
    // File 菜单
    wxMenu* menuFile = new wxMenu;
    menuFile->Append(wxID_NEW, "&New\tCtrl-N");
    menuFile->Append(wxID_OPEN, "&Open...\tCtrl-O");
    menuFile->Append(wxID_SAVE, "&Save\tCtrl-S");
    menuFile->Append(wxID_SAVEAS, "Save &As...");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT, "E&xit\tAlt-F4");

    // Edit 菜单
    wxMenu* menuEdit = new wxMenu;
    menuEdit->Append(wxID_UNDO, "&Undo\tCtrl-Z");
    menuEdit->Append(wxID_REDO, "&Redo\tCtrl-Y");
    menuEdit->AppendSeparator();
    menuEdit->Append(wxID_COPY, "&Copy\tCtrl-C");
    menuEdit->Append(wxID_PASTE, "&Paste\tCtrl-V");

    // View 菜单
    wxMenu* menuView = new wxMenu;
    menuView->Append(wxID_ZOOM_IN, "Zoom &In\tCtrl-+");
    menuView->Append(wxID_ZOOM_OUT, "Zoom &Out\tCtrl--");
    menuView->AppendCheckItem(ID_SHOW_STATUSBAR, "Show Status Bar")->Check(true);

    // Help 菜单
    wxMenu* menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT, "&About\tF1");

    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuEdit, "&Edit");
    menuBar->Append(menuView, "&View");
    menuBar->Append(menuHelp, "&Help");
    SetMenuBar(menuBar);

    CreateStatusBar();
    SetStatusText("准备就绪");

    wxToolBar* toolBar = CreateToolBar();
    toolBar->AddTool(wxID_NEW, "新建", wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR));
    toolBar->AddTool(wxID_OPEN, "打开", wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR));
    toolBar->AddTool(wxID_SAVE, "保存", wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR));
    toolBar->AddSeparator();
    toolBar->AddTool(wxID_UNDO, "撤销", wxArtProvider::GetBitmap(wxART_UNDO, wxART_TOOLBAR));
    toolBar->AddTool(wxID_REDO, "重做", wxArtProvider::GetBitmap(wxART_REDO, wxART_TOOLBAR));
    toolBar->AddSeparator();
    toolBar->AddTool(wxID_COPY, "复制", wxArtProvider::GetBitmap(wxART_COPY, wxART_TOOLBAR));
    toolBar->AddTool(wxID_PASTE, "粘贴", wxArtProvider::GetBitmap(wxART_PASTE, wxART_TOOLBAR));
    toolBar->AddSeparator();
    toolBar->AddTool(wxID_ABOUT, "关于", wxArtProvider::GetBitmap(wxART_HELP, wxART_TOOLBAR));
    toolBar->Realize();

    m_splitter = new wxSplitterWindow(this, wxID_ANY);
    m_treeCtrl = new wxTreeCtrl(m_splitter, wxID_ANY, wxDefaultPosition, wxSize(200, -1),
        wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_DEFAULT_STYLE);
    m_drawPanel = new MyDrawPanel(m_splitter);
    m_splitter->SplitVertically(m_treeCtrl, m_drawPanel, 200);
    m_splitter->SetSashGravity(0.0);

    wxTreeItemId root = m_treeCtrl->AddRoot("组件库");
    wxTreeItemId logicId = m_treeCtrl->AppendItem(root, "逻辑门");
    wxTreeItemId outputId = m_treeCtrl->AppendItem(root, "输出");
    m_treeCtrl->AppendItem(logicId, "AND");
    m_treeCtrl->AppendItem(logicId, "OR");
    m_treeCtrl->AppendItem(logicId, "NOT");
    m_treeCtrl->AppendItem(outputId, "LED");
    m_treeCtrl->ExpandAll();

    m_treeCtrl->Bind(wxEVT_TREE_SEL_CHANGED, [this](wxTreeEvent& event) {
        wxTreeItemId item = event.GetItem();
        if (!item.IsOk()) return;
        wxString name = m_treeCtrl->GetItemText(item);
        if (name != "逻辑门" && name != "输出" && name != "组件库") {
            m_drawPanel->AddShape(name);
        }
        });

    UpdateTitle();
}

void MyFrame::UpdateTitle() {
    SetTitle("Project1 - wxWidgets 绘图Demo");
}

// ===== 菜单实现 =====
void MyFrame::OnNew(wxCommandEvent&) { m_drawPanel->ClearAll(); m_currentFile.Clear(); }
void MyFrame::OnOpen(wxCommandEvent&) {
    wxFileDialog openFileDialog(this, _("打开文件"), "", "",
        "Text files (*.txt)|*.txt", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL) return;
    m_drawPanel->LoadFromFile(openFileDialog.GetPath());
    m_currentFile = openFileDialog.GetPath();
}
void MyFrame::OnSave(wxCommandEvent&) {
    if (m_currentFile.IsEmpty()) {
        OnSaveAs(wxCommandEvent());
    }
    else {
        m_drawPanel->SaveToFile(m_currentFile);
    }
}
void MyFrame::OnSaveAs(wxCommandEvent&) {
    wxFileDialog saveFileDialog(this, _("另存为"), "", "",
        "Text files (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (saveFileDialog.ShowModal() == wxID_CANCEL) return;
    m_currentFile = saveFileDialog.GetPath();
    m_drawPanel->SaveToFile(m_currentFile);
}
void MyFrame::OnQuit(wxCommandEvent&) { Close(true); }

void MyFrame::OnUndo(wxCommandEvent&) { m_drawPanel->Undo(); }
void MyFrame::OnRedo(wxCommandEvent&) { m_drawPanel->Redo(); }
void MyFrame::OnCopy(wxCommandEvent&) { m_drawPanel->Copy(); }
void MyFrame::OnPaste(wxCommandEvent&) { m_drawPanel->Paste(); }

void MyFrame::OnZoomin(wxCommandEvent&) { m_drawPanel->ZoomIn(); }
void MyFrame::OnZoomout(wxCommandEvent&) { m_drawPanel->ZoomOut(); }

void MyFrame::OnToggleStatusBar(wxCommandEvent& evt) {
    if (evt.IsChecked()) {
        if (!GetStatusBar()) CreateStatusBar();
        GetStatusBar()->Show();
        SetStatusText("状态栏已启用");
    }
    else {
        if (GetStatusBar()) GetStatusBar()->Hide();
    }
    Layout();
}
void MyFrame::OnAbout(wxCommandEvent&) {
    wxMessageBox("Project1 - wxWidgets 简易绘图器\n作者：Rachel",
        "关于", wxOK | wxICON_INFORMATION);
}

// ===== 应用入口 =====
class MyApp : public wxApp {
public:
    virtual bool OnInit() override {
        MyFrame* frame = new MyFrame("wxWidgets 绘图Demo");
        frame->Show(true);
        return true;
    }
};
wxIMPLEMENT_APP(MyApp);

