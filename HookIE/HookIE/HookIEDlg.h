
// HookIEDlg.h : 头文件
//

#pragma once
#include "afxwin.h"


// CHookIEDlg 对话框
class CHookIEDlg : public CDialogEx
{
// 构造
public:
	CHookIEDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_HOOKIE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnHookie();
	afx_msg void OnBnClickedBtnUnhookie();
private:
	CButton m_uiHookIEBtn;
	CButton m_uiUnHookIEBtn;
};
