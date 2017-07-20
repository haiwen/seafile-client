#ifndef SEAFILE_CLIENT_SEARCH_BAR_H_
#define SEAFILE_CLIENT_SEARCH_BAR_H_

#include <QWidget>
#include <QLineEdit>

class QToolButton;
class QLabel;

class SearchBar : public QLineEdit
{
    Q_OBJECT
public:
    SearchBar(QWidget *parent=0);
    void setPlaceholderText(const QString& text);

private slots:
    void onTextChanged(const QString& text);

private:
    Q_DISABLE_COPY(SearchBar)

    void paintEvent(QPaintEvent* event);
    void resizeEvent(QResizeEvent* event);

    int clear_button_size_;
    QToolButton *clear_button_;
    QLabel *placeholder_label_;
};

#endif // SEAFILE_CLIENT_SEARCH_BAR_H_
