#include "const.h"
#include "v4l2.h"
#include "mpp.h"
#include "SharedQueue.h"
#include "H265LiveServerMediaSubsession.h"
#include "H264LiveServerMediaSubsession.h"
#include "EncodeThread.h"

EventLoopWatchVariable watchVariable;
std::atomic<bool> exit_flag;

void handle(int signum)
{
    watchVariable = 1;
    exit_flag = true;
}

int main()
{

    watchVariable = 0;
    exit_flag = false;


    BasicTaskScheduler *scheduler = BasicTaskScheduler::createNew();
    BasicUsageEnvironment *env = BasicUsageEnvironment::createNew(*scheduler);

    RTSPServer *server = RTSPServer::createNew(*env, RTSP_PORT);

    //主码流 H.265 session
    SharedQueue *shared_queue_main = new SharedQueue();
    ServerMediaSession *session_main = ServerMediaSession::createNew(*env, "h265");
    H265LiveServerMediaSubsession *subsession_main = H265LiveServerMediaSubsession::createNew(shared_queue_main, *env, True);
    session_main->addSubsession(subsession_main);
    server->addServerMediaSession(session_main);

    // 子码流 H.264 session
    SharedQueue *shared_queue_sub = new SharedQueue();
    ServerMediaSession *session_sub = ServerMediaSession::createNew(*env, "h264");
    H264LiveServerMediaSubsession *subsession_sub = H264LiveServerMediaSubsession::createNew(shared_queue_sub, *env, True);
    session_sub->addSubsession(subsession_sub);
    server->addServerMediaSession(session_sub);

    std::thread encode_h265(start_encode_h265, shared_queue_main, scheduler);
    std::thread encode_h264(start_encode_h264, shared_queue_sub, scheduler);
    std::cout << "双编码线程启动" << std::endl;

    signal(SIGINT, handle);
    signal(SIGTERM, handle);
    signal(SIGHUP, handle);

    std::cout << "RTSP Server 启动" << std::endl;
    std::cout << "主码流: rtsp://192.168.100.1:8554/h265" << std::endl;
    std::cout << "子码流: rtsp://192.168.100.1:8554/h264" << std::endl;
    env->taskScheduler().doEventLoop(&watchVariable);

    std::cout << "收到退出信号，开始清理资源..." << std::endl;

    //清理v4l2/mpp资源
    encode_h265.join();
    encode_h264.join();
    std::cout << "编码线程已退出" << std::endl;

    //清理 live555 资源
    Medium::close(server);
    env->reclaim();
    delete scheduler;

    //释放共享队列
    delete shared_queue_main;
    delete shared_queue_sub;

    std::cout << "所有资源释放完成，程序退出" << std::endl;
    return 0;
}
