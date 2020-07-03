运行： 配置 xtserver.app.conf 中 ip.bind ip.map
           启动 xtServer.exe  
           运行 xtClient.exe

采集发布：ContextPublisher::startCapture
	建立2个线程，编码线程onThreadEncoder   发送线程onThreadSender
	将VideoSource添加到采集器中（VideoCapture::setSink），采集到原始数据后 调用VideoSource的onFrame处理（转码为YUV420并水平翻转）
	将YUV数据发送给MediaSinkHolder，以便在编码线程中进行编码，同时直接在 CD3D9Render 中显示，见VideoSource::addSink
	
	编码线程
	从MediaSinkHolder获取一帧YUV数据，调用VideoEncoder::inputFrame编码，编码为h264，回调数据放到循环队列StreamBuffer中

	发送线程
	从循环队列StreamBuffer中取数据，通过MissionObject发送数据。如果设置了rollback，也发给rollback

	网路库MissionObject封装处理：
	1. 建立XT_ROOM_TYPE_LIVE房间， 建立本地对象（xt_create_local_object）进行发流处理（如果连麦，需要增加收流处理流程），
	    建立网络对象xt_create_network_object，通过该对象建立服务器和本地对象之间的桥梁，将本地对象数据发往服务器，将服务器数据发给本地对象
	2. 发送请求发布消息给服务器PRO_CMD_PUBLISH_START_REQ
	3. 收到PRO_CMD_PUBLISH_START_RSP回复后MissionObject::onResponse处理
	4. 设置服务器地址xt_network_set_ip4_udp/xt_network_set_ip4_tcp
	对于直播类 不管连接是否已建立 都可以通过本地对象发送数据。

解码播放：ContextSubscriber::startShow
	建立2个线程：解码线程onThreadDecoder 渲染线程onThreadRender
	MissionObject接收到数据后，调用ContextSubscriber::onHeader/ContextSubscriber::onData处理，将数据放到循环队列StreamBuffer中

	解码线程
	从循环队列StreamBuffer中取数据，调用VideoDecoder::Decode解码，解码YUV数据放到MediaFrameHolder::onFrame中，以便进行显示

	渲染线程
	通过MediaFrameHolder::getFrameLists获取原始数据，然后在CD3D9Render 中显示

	网路库MissionObject封装处理：
	1. 建立XT_ROOM_TYPE_LIVE房间， 建立本地对象（xt_create_local_object）进行收流处理（如果连麦，需要增加发流处理流程），
	    建立网络对象xt_create_network_object，通过该对象建立服务器和本地对象之间的桥梁，将本地对象数据发往服务器，将服务器数据发给本地对象
	2. 发送请求订阅消息给服务器PRO_CMD_SUBSCRIBE_START_REQ
	3. 收到PRO_CMD_SUBSCRIBE_START_REQ_RSP回复后MissionObject::onResponse处理
	4. 设置服务器地址xt_network_set_ip4_udp/xt_network_set_ip4_tcp
	

服务器端：
	NetListener::doRecv 接收客户端连接，然后通过NetClientManager创建NetClient对象
	NetClient::doRecv 客户端消息收发事件
	onStartPublish::发布端请求发布数据，通过MissionMgr::startPublisher处理，采用推流模式
		创建XT_ROOM_TYPE_LIVE房间（如果需要连麦，则建立XT_ROOM_TYPE_METTING视频会议房间），建立网络对象xt_create_network_object
		设置对端信息xt_network_set_ip4_udp/xt_network_set_ip4_tcp 然后回复发布端
	
	onStartSubscribe::客户端订阅发布端数据流，通过MissionMgr::startSubscriber处理
		建立网络对象xt_create_network_object，并加入发布端房间。	
		设置对端信息xt_network_set_ip4_udp/xt_network_set_ip4_tcp 然后回复客户端

	如果是视频会议等情况，则数据流的streamid比须不同，建议由主持方进行分配。
	

