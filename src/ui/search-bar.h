#ifndef SEAFILE_CLIENT_SEARCH_BAR_H_
#define SEAFILE_CLIENT_SEARCH_BAR_H_

#include <QWidget>
#include <QLineEdit>

class QToolButton;

class SearchBar : public QLineEdit
{
    Q_OBJECT
public:
    SearchBar(QWidget *parent=0);
    void paintEvent(QPaintEvent* event);
    void resizeEvent(QResizeEvent* event);

private:
    Q_DISABLE_COPY(SearchBar)

    QToolButton *clear_button_;
    int clear_button_size_;

private slots:
    void updateClearButton(const QString& text);
};

#endif // SEAFILE_CLIENT_SEARCH_BAR_H_
