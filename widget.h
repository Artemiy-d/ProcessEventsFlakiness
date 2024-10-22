#include <QPushButton>
#include <QThread>

#include <memory>

class Worker;
class QThread;

struct DeleteLater
{
    template<typename T>
    void operator () (T* obj) const {
        obj->deleteLater();
    }
};

class Widget : public QPushButton
{
public:
    Widget();
    ~Widget() override;
private:
    void start();
    void stop();
private:
    std::unique_ptr<Worker, DeleteLater> m_worker;
    QThread m_thread;
};
