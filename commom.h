#ifndef COMMOM_H
#define COMMOM_H
#include <QWidget>
#include <QMainWindow>
#include <QStringListModel>
#include <QProcess>
#include <QTextEdit>
#include <QTextStream>

#define de qDebug()

inline QString ConvertTextToHtml(QString t)
{
    return t.replace('<',"&lt;").replace('<',"&gt;").replace("\r\n","<br/>").replace('\n',"<br/>").replace('\r',"<br/>")
            .replace(' ',"&nbsp;").replace('\b',"&nbsp;&nbsp;&nbsp;&nbsp;");
    return t;
}

#define PLAIN_TEXT(text) QString("<text style=\" color:#ffffff;\">"+ConvertTextToHtml(text)+"</text>")
#define RED_TEXT(text) QString("<text style=\" color:#ff4525;\">"+ConvertTextToHtml(text)+"</text>") //
#define GREEN_TEXT(text) QString("<text style=\" color:#44ff55;\">"+ConvertTextToHtml(text)+"</text>")
#define PURPLE_TEXT(text) QString("<text style=\" color:#ea76ff;\">"+ConvertTextToHtml(text)+"</text>")
#define BLUE_TEXT(text) QString("<text style=\" color:#0525f2;\">"+ConvertTextToHtml(text)+"</text>")
#define WHITE_TEXT(text) QString("<text style=\" color:#f2f2f2;\">"+ConvertTextToHtml(text)+"</text>")
#define SKYBLUE_TEXT(text) QString("<text style=\" color:rgb(44,235,255);\">"+ConvertTextToHtml(text)+"</text>")
#define YELLOW_TEXT(text) QString("<text style=\" color:rgb(241,214,110);\">"+ConvertTextToHtml(text)+"</text>")


#define Update_Style(widget_ptr) (widget_ptr)->style()->polish(widget_ptr);


inline QString fmt(const char* mode)
{
    return mode;
}
template<class T>
inline QString str(const T& value)
{
    return std::to_string(value).c_str();
}
template <typename Type>
QString PrintOneFormated(const char* format, const Type& value,int& _index)
{
    char buffer[16];
    char out_buffer[168];
    int index = 0;
    while (format[index] != '}' && format[index] != '\0' && format[index] != ' ')
    {
        buffer[index++] = format[index];
        if (index > 15) return 0;
    }
    buffer[index] = '\0';
    _index = index;

    int count = sprintf(out_buffer,buffer, value);
    out_buffer[count]='\0';
    return QString(out_buffer);
}

// Print("I have {} pens, every one sells for {%.2f} yuan.",10,2.589)
// output: I have 10 pens, every one sells for 2.59 yuan.
// Print("\\{\\}") can print a brace. Escape character can help print some key character.
// Return true if perfectly print, that is : every value conbined with a {}.
template <typename Type>
QString fmt(const char* mode, const Type& arg)
{
    QString out;
    while (true)
    {
        if (*mode == '\0') return out;
        if (*mode == '{') break;
        if (*mode == '\\') ++mode;
        out += *mode;
        ++mode;
    }
    ++mode;

    if (*mode == '%')  // %.2f
    {
        int size;
        QString s = PrintOneFormated(mode, arg,size);
        if (size == 0)
        {
            return out + mode;
        }
        mode += size + 1;
        out += s;
    }
    else if (*mode == '}')  // {}
    {
        out += str(arg);
        ++mode;
    }
    else  // unknown format...
    {
        return out + mode;
    }
    return out + mode;
}

// Print("I have {} pens, every one sells for {%.2f} yuan.",10,2.589)
// output: I have 10 pens, every one sells for 2.59 yuan.
// Print("\\{\\}") can print a brace. Escape character can help print some key characters.
// Return true if perfectly print, that is : every value conbined with a {}.
template<class Type, class ...Types>
QString fmt(const char* mode,const Type& arg, const Types& ...args)
{
    QString out;
    while (true)
    {
        if (*mode == '\0') return out;
        if (*mode == '{') break;
        if (*mode == '\\') ++mode;
        out += *mode;
        ++mode;
    }
    ++mode;

    if (*mode == '%')  // %.2f
    {
        int index = 0;
        QString s = PrintOneFormated(mode, arg, index);
        if (s.size() == 0)
        {
            return out + mode;
        }
        mode += index + 1;
        out += s;
    }
    else if (*mode == '}')  // {}
    {
        out += str(arg);
        ++mode;
    }
    else  // unknown format...
    {
        out+= mode;
        return out;
    }
    return out + fmt(mode, args...);
}


#endif // COMMOM_H
