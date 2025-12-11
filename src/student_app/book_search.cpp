#include <seatui/student/book_search.hpp>
#include <QFile>// for file reading
#include <QTextStream>//for text stream
#include <QStringConverter>//UTF-8 encoding
#include <QDebug>  // drbugging
#include <QDir>    // directory and path handling

#include <QStringConverter>   // for setting text encoding

#include <QFileInfo>// for file existence check






// if haystack contains needle (capital insensitive)   
static inline bool containsCaseInsensitive(const QString& haystack, const QString& needle) {
    return haystack.contains(needle, Qt::CaseInsensitive);
}

QString BookSearchEngine::trim(const QString& s) {
    QString t = s;
    return t.trimmed();// get rid of the blanks in the beggining and the end
}



bool BookSearchEngine::loadFromFile(const QString& filePath, QString* errMsg) {
    QString p = filePath;
    if (p.isEmpty()) {
        if (errMsg) *errMsg = QStringLiteral("未提供文件路径。");
        loaded_ = false;// not loaded state
        items_.clear();//clear the book liss
        return false;
    }
    if (!QFileInfo::exists(p)) {
        if (errMsg) *errMsg = QStringLiteral("文件不存在：%1").arg(p);
        loaded_ = false;
        items_.clear();
        return false;
    }

    //use readonly and test mode to open
    QFile f(p);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errMsg) *errMsg = QStringLiteral("无法打开文件：%1").arg(p);
        loaded_ = false;
        items_.clear();
        return false;
    }

    QTextStream in(&f);//create text stream
    in.setEncoding(QStringConverter::Utf8); // Qt6- UTF-8 encoding

    items_.clear();
    BookItem cur;
    bool hasData = false;

    while (!in.atEnd()) {
        const QString raw = in.readLine();
        const QString line = trim(raw);
        // empty line indicates end of one book item
        if (line.isEmpty()) {
            if (hasData && !cur.title.isEmpty()) {
                items_.push_back(cur);//put the book inside the list
                cur = BookItem();// reset for next book to push in
            }
            hasData = false;
            continue;
        }
        // deal with TItle: title line
        int pos = line.indexOf(':');
        if (pos > 0) {
            // key:value pair
            const QString key = trim(line.left(pos));//like'ISBN'
            const QString val = trim(line.mid(pos + 1));//like '978-7-302-46715-7'
            hasData = true;

            if      (key == "ISBN")       cur.isbn = val;
            else if (key == "Title")      cur.title = val;
            else if (key == "Author")     cur.author = val;
            else if (key == "Publisher")  cur.publisher = val;
            else if (key == "Date")       cur.date = val;
            else if (key == "Category")   cur.category = val;
            else if (key == "CallNumber") cur.callNumber = val;
            else if (key == "Total")      cur.total = val.toInt();//change it into an integer
            else if (key == "Available")  cur.available = val.toInt();
        }
    }
    // check if the last book is pushed
    if (hasData && !cur.title.isEmpty()) items_.push_back(cur);

    loaded_ = true;
    return true;
}


QVector<BookItem> BookSearchEngine::searchByKeyword(const QString& keyword) const {
    QVector<BookItem> out;//to store the search results
    if (!loaded_) return out;
    const QString k = keyword.trimmed();//if the keyword is empty, then return the empty list
    if (k.isEmpty()) return out;
// let the searcher can search author or the title
    for (const auto& b : items_) {
        if (containsCaseInsensitive(b.title, k) || containsCaseInsensitive(b.author, k)) {
            out.push_back(b);
        }
    }
    return out;
}
