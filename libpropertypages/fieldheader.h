#pragma once
class WXEXPORT FieldHeader :	public PropertyPageBase
{
public:
    FieldHeader(wxWindow *parent);
    ~FieldHeader(void);
    wxTextCtrl *GetLabelCtrl();
    wxTextCtrl *GetHeadingCtrl();
protected:
    void do_layout();
    void set_properties();
private:
    wxStaticText *m_label1, *m_label2, *m_label3, *m_label4;
    wxTextCtrl *m_label, *m_heading;
    wxComboBox *m_labelPos, *m_headingPos;
};

