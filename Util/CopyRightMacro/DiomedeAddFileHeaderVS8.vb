Imports System
Imports EnvDTE
Imports EnvDTE80
Imports EnvDTE90
Imports System.Diagnostics

Public Module DiomedeAddFileHeaderVS8

    Public indentStyle As String
    Public tabSize As String
    Public indentSize As String
    Public insertTabs As Boolean

    Public keys As String() = { _
          "Basic", _
          "C/C++", _
          "CSharp", _
          "CSS", _
          "HTML", _
          "PL/SQL", _
          "PlainText", _
          "T-SQL", _
          "XML"}



    Sub FileHeader()
        Dim doc As Document
        Dim docName As String
        Dim companyName As String = " Diomede Corporation"
        Dim authorName As String = "Patty Harris"
        Dim copyrightTextLine1 As String = "(C) Copyright 2010"
        Dim copyrightTextLine2a As String = "Use"
        Dim copyrightTextLine2b As String = " modification"
        Dim copyrightTextLine2c As String = " and distribution is subject to   "
        Dim copyrightTextLine3 As String = "the New BSD License (See accompanying file LICENSE)."
        Dim emptyLineText As String = " * "

        ' This oddity fixes a problem with Visual Studio and the usage of commas
        ' in text (stupid).
        Dim comma As Char = ","

        ' Get the file language - as a default, set it to c++
        Dim fileLanguage As String = keys(1)
        Dim tmpGileLanguage As String = GetNetLanguage()
        If tmpGileLanguage <> "" Then
            fileLanguage = tmpGileLanguage
        End If

        ' Get the value of the enviroment tabs and spaces.
        GetTabsAndSpaces(fileLanguage)

        ' Set the editor spacing to none
        NoTabsAndSpaces(fileLanguage)

        ' Are we at the top of the file?
        Dim caretPosition As TextPoint = DTE.ActiveDocument.Selection.ActivePoint

        ' Get the name of this object from the file name
        doc = DTE.ActiveDocument

        ' Get the name of the current document
        docName = doc.Name

        ' Set selection to top of document
        DTE.ActiveDocument.Selection.StartOfDocument()
        DTE.ActiveDocument.Selection.NewLine()

        ' Write first line
        DTE.ActiveDocument.Selection.LineUp()
        DTE.ActiveDocument.Selection.Text = "/*********************************************************************"
        DTE.ActiveDocument.Selection.NewLine()

        DTE.ActiveDocument.Selection.Text = emptyLineText
        DTE.ActiveDocument.Selection.NewLine()

        ' Write copyright tag
        DTE.ActiveDocument.Selection.Text = " *  file:  " + docName
        DTE.ActiveDocument.Selection.NewLine()

        DTE.ActiveDocument.Selection.Text = emptyLineText
        DTE.ActiveDocument.Selection.NewLine()

        DTE.ActiveDocument.Selection.Text = " *  " + copyrightTextLine1 + comma + companyName
        DTE.ActiveDocument.Selection.NewLine()
        DTE.ActiveDocument.Selection.Text = " *  All rights reserved"
        DTE.ActiveDocument.Selection.NewLine()

        DTE.ActiveDocument.Selection.Text = emptyLineText
        DTE.ActiveDocument.Selection.NewLine()

        DTE.ActiveDocument.Selection.Text = " *  " + copyrightTextLine2a + comma + copyrightTextLine2b + comma + copyrightTextLine2c
        DTE.ActiveDocument.Selection.NewLine()
        DTE.ActiveDocument.Selection.Text = " *  " + copyrightTextLine3
        DTE.ActiveDocument.Selection.NewLine()
        DTE.ActiveDocument.Selection.Text = emptyLineText
        DTE.ActiveDocument.Selection.NewLine()

        ' Write author name tag (optional)
        DTE.ActiveDocument.Selection.Text = " * Purpose: "
        DTE.ActiveDocument.Selection.NewLine()
        DTE.ActiveDocument.Selection.Text = emptyLineText
        DTE.ActiveDocument.Selection.NewLine()

        ' Could write <email></email> tag
        ' Could write <date></date> tag
        ' Could write <summary></summary> tag

        ' Write last line
        DTE.ActiveDocument.Selection.Text = emptyLineText
        DTE.ActiveDocument.Selection.NewLine()
        DTE.ActiveDocument.Selection.Text = " *********************************************************************/"
        DTE.ActiveDocument.Selection.NewLine()

        ' Reset the value of environment tabs and spaces.
        ResetTabsAndSpaces(fileLanguage)

    End Sub

    ' Get the tabs and spacing to allow us to reset them when we're finished.
    Sub GetTabsAndSpaces(ByVal fileLanguage As String)
        Dim textEditor As Properties
        textEditor = DTE.Properties("TextEditor", fileLanguage)

        indentStyle = textEditor.Item("IndentStyle").Value
        tabSize = textEditor.Item("TabSize").Value
        indentSize = textEditor.Item("IndentSize").Value
        insertTabs = textEditor.Item("InsertTabs").Value

    End Sub

    ' Turns off tabs and spaces, otherwise causing odd formatting behavior
    Sub NoTabsAndSpaces(ByVal fileLanguage As String)
        Dim textEditor As Properties
        textEditor = DTE.Properties("TextEditor", fileLanguage)

        textEditor.Item("IndentStyle").Value = vsIndentStyle.vsIndentStyleNone
        textEditor.Item("TabSize").Value = 1
        textEditor.Item("IndentSize").Value = 1
        textEditor.Item("InsertTabs").Value = False
    End Sub

    ' Reset the tabs and spaces back to their original value.
    Sub ResetTabsAndSpaces(ByVal fileLanguage As String)
        Dim textEditor As Properties
        textEditor = DTE.Properties("TextEditor", fileLanguage)

        textEditor.Item("IndentStyle").Value = indentStyle
        textEditor.Item("TabSize").Value = tabSize
        textEditor.Item("IndentSize").Value = indentSize
        textEditor.Item("InsertTabs").Value = insertTabs
    End Sub

    Private Function GetNetLanguage() As String

        Dim sLanguageGuid As String
        Dim fileCodeModel As FileCodeModel
        Dim codeModel As CodeModel
        Dim sLanuageConst = ""

        fileCodeModel = DTE.ActiveDocument.ProjectItem.FileCodeModel
        If Not fileCodeModel Is Nothing Then
            sLanguageGuid = DTE.ActiveDocument.ProjectItem.FileCodeModel.Language
        Else
            codeModel = DTE.ActiveDocument.ProjectItem.ContainingProject.CodeModel

            If Not codeModel Is Nothing Then
                sLanguageGuid = DTE.ActiveDocument.ProjectItem.ContainingProject.CodeModel.Language
            End If
        End If

        If sLanguageGuid = "" Then
            Return sLanguageGuid
        End If

        Try

            ' Get the language of a project
            ' sLanguageGuid = DTE.ActiveDocument.ProjectItem.ContainingProject.CodeModel.Language

            ' Get the language of a file
            ' sLanguageGuid = DTE.ActiveDocument.ProjectItem.FileCodeModel.Language

            Select Case sLanguageGuid

                Case EnvDTE.CodeModelLanguageConstants.vsCMLanguageVB
                    sLanuageConst = keys(0)
                Case EnvDTE.CodeModelLanguageConstants.vsCMLanguageCSharp
                    sLanuageConst = keys(2)
                Case EnvDTE.CodeModelLanguageConstants.vsCMLanguageVC
                    sLanuageConst = keys(1)
                Case EnvDTE.CodeModelLanguageConstants.vsCMLanguageMC
                    sLanuageConst = keys(1)
            End Select

        Catch objException As System.Exception
            MsgBox(objException.ToString)
        End Try

        Return sLanuageConst

    End Function

End Module

