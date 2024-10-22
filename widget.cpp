#include "widget.h"

#include <QApplication>
#include <QTimer>
#include <QMetaObject>
#include <QDebug>

/*

The snippet shows a race condition cauing UB.

Description of unlucky flow causing UB:
- a user clicks "stop" and posts "deleteLater" of the worker to the thread.
- "deleteLater" is enqueued to the event loop of the thread in a time window
  between start QTimer::timeout invokation and deleting of the AudioSink.
- QApplication::processEvents in the destructor of AudioSink performs
  the enqueued deleting of the Worker.
- m_audioSink = createSink() assigns the unique_ptr to already freed memory,
  what causes UB: a memoty leak, crash, or memory corruption.

An example of good logs:

Restart audio begin
Sink deleted: QObject(0x6000034a15d0)
Sink created: QObject(0x60000349e950)
Restart audio end
Restart audio begin
Sink deleted: QObject(0x60000349e950)
Sink created: QObject(0x60000349b2c0)
Restart audio end
Worker destroyed: QObject(0x6000036df5c0)
Sink deleted: QObject(0x60000349b2c0) // The sink is destroyed upon destruction of Worker.
                                      // No sinks are created anymore unless a user starts a new session.

An example of bad logs showing a memry leak:

Restart audio begin
Sink deleted: QObject(0x60000349e920)
Sink created: QObject(0x60000349e920)
Restart audio end
Restart audio begin
Worker destroyed: QObject(0x6000036c4d20) // Worker is destroyed when processEvents in ~AudioSink performs deferred delete
Sink deleted: QObject(0x60000349e920)
Sink created: QObject(0x60000349e920) // The created sink is assigned to the deleted member of deleted Worker,
                                      // that means the member won't be deleted again.
                                      // In general case, it's UB, in the specific case, it's a memory leak.
Restart audio end

*/

class AudioSink : public QObject
{
public:
    AudioSink() = default;
    ~AudioSink() override
    {
        // QApplication::processEvents causes UB
        QApplication::processEvents();
    }
};

class Worker : public QObject
{
public:
    Worker()
    {
        qDebug() << "Worker created:" << this;
    }

    ~Worker()
    {
        qDebug() << "Worker destroyed:" << this;
    }

    void initAudio()
    {
        Q_ASSERT(thread() == QThread::currentThread());

        qDebug() << "Init audio";

        auto timer = new QTimer(this);
        timer->setTimerType(Qt::PreciseTimer);
        timer->callOnTimeout(this, &Worker::restartAudio);

        // The less interval, the more probability to catch the race condition
        timer->start(std::chrono::milliseconds(8));
    }

private:
    void restartAudio()
    {
        // This place is an unlucky time window.
        // If worker->deleteLater is posted to the thread between start of QTimer::timeout
        // and m_audioSink.reset(), the UB occurs.
        //
        // Uncomment the sleep to increase the time window
        // QThread::sleep(std::chrono::milliseconds(3));

        qDebug() << "Restart audio begin";

        m_audioSink.reset();

        m_audioSink = createSink();

        qDebug() << "Restart audio end";
    }

    static std::unique_ptr<AudioSink> createSink()
    {
        auto sink = std::make_unique<AudioSink>();
        qDebug() << "Sink created:" << sink.get();

        connect(sink.get(), &QObject::destroyed, sink.get(), [](QObject* sink) {
            qDebug() << "Sink deleted:" << sink;
        });

        return sink;
    }

private:
    std::unique_ptr<AudioSink> m_audioSink;
};

Widget::Widget()
{
    m_thread.start();

    resize(200, 200);
    stop();
}

Widget::~Widget()
{
    m_worker.reset();
    m_thread.quit();
    m_thread.wait();
}

void Widget::start()
{
    Q_ASSERT(!m_worker);

    m_worker.reset(new Worker);
    m_worker->moveToThread(&m_thread);
    QMetaObject::invokeMethod(m_worker.get(), &Worker::initAudio);

    setText(QStringLiteral("STOP"));
    connect(this, &QPushButton::clicked, this, &Widget::stop, Qt::SingleShotConnection);
}

void Widget::stop()
{
    m_worker.reset();
    setText(QStringLiteral("START"));
    connect(this, &QPushButton::clicked, this, &Widget::start, Qt::SingleShotConnection);
}
