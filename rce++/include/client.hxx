/*
 *    client.hxx
 *
 *    This file is part of the RoboEarth Cloud Engine rce++ client.
 *
 *    This file was originally created for RoboEearth
 *    http://www.roboearth.org/
 *
 *    The research leading to these results has received funding from
 *    the European Union Seventh Framework Programme FP7/2007-2013 under
 *    grant agreement no248942 RoboEarth.
 *
 *    Copyright 2012 RoboEarth
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 *     \author/s: Dominique Hunziker
 */

#ifndef CLIENT_HXX_
#define CLIENT_HXX_

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <curl/curl.h>

#include "types.hxx"
#include "handler.hxx"

#define CLIENT_VERSION "20130131"

namespace rce
{

enum interface_t
{
	INTERFACE_SERVICE_CLIENT,
	INTERFACE_SERVICE_PROVIDER,
	INTERFACE_PUBLISHER,
	INTERFACE_SUBSCRIBER,
	CONVERTER_SERVICE_CLIENT,
	CONVERTER_SERVICE_PROVIDER,
	CONVERTER_PUBLISHER,
	CONVERTER_SUBSCRIBER,
	FORWARDER_SERVICE_CLIENT,
	FORWARDER_SERVICE_PROVIDER,
	FORWARDER_PUBLISHER,
	FORWARDER_SUBSCRIBER
};

class ClientException: public std::runtime_error
{
	public:
		ClientException(const std::string &e) :
				std::runtime_error::runtime_error(e)
		{
		}
};

template<class Client>
class InterfaceBase_impl
{
	public:
		InterfaceBase_impl(const typename Client::ProtocolPtr_t protocol,
				const typename Client::String tag,
				const typename Client::String type) :
				_protocol(protocol), _tag(tag), _type(type)
		{
		}

	protected:
		const typename Client::ProtocolPtr_t _protocol;

		const typename Client::String _tag;
		const typename Client::String _type;

};

template<class Client>
class ReceiverInterface_impl: public InterfaceBase_impl<Client>
{
	public:
		ReceiverInterface_impl(const typename Client::ProtocolPtr_t protocol,
				const typename Client::String tag,
				const typename Client::String type,
				const typename Client::MsgCallback_t cb) :
				InterfaceBase_impl<Client>(protocol, tag, type), _cb(cb)
		{
		}

		virtual void receive(const typename Client::String &type,
				const typename Client::Value &msg,
				const typename Client::String &msgID) = 0;

		virtual ~ReceiverInterface_impl()
		{
		}

	protected:
		const typename Client::MsgCallback_t _cb;
};

template<class Client>
class ServiceClient_impl: public ReceiverInterface_impl<Client>
{
	private:
		typedef std::pair<typename Client::String,
				typename Client::MsgCallback_t> _CallbackRef_t;
		typedef std::vector<_CallbackRef_t> _CallbackRefVector_t;

	public:
		ServiceClient_impl(const typename Client::ProtocolPtr_t protocol,
				const typename Client::String tag,
				const typename Client::String srvType,
				const typename Client::MsgCallback_t cb) :
				ReceiverInterface_impl<Client>(protocol, tag, srvType, cb)
		{
			ReceiverInterface_impl<Client>::_protocol.registerInterface(
					ReceiverInterface_impl<Client>::_tag, this);
		}

		~ServiceClient_impl()
		{
			ReceiverInterface_impl<Client>::_protocol.unregisterInterface(
					ReceiverInterface_impl<Client>::_tag, this);
		}

		/*
		 * Call Service.
		 *
		 * @param cb:	Overwrite default callback for one service call.
		 */
		void call(const typename Client::Value &msg);
		void call(const typename Client::Value &msg,
				const typename Client::MsgCallback_t &cb);

		void receive(const typename Client::String &type,
				const typename Client::Value &msg,
				const typename Client::String &msgID);

	private:
		_CallbackRefVector_t _callbacks;
};

template<class Client>
class Publisher_impl: public InterfaceBase_impl<Client>
{
	public:
		Publisher_impl(const typename Client::ProtocolPtr_t protocol,
				const typename Client::String tag,
				const typename Client::String msgType) :
				InterfaceBase_impl<Client>(protocol, tag, msgType)
		{
		}

		/*
		 * Publish message.
		 */
		void publish(const typename Client::Value &msg);
};

template<class Client>
class Subscriber_impl: public ReceiverInterface_impl<Client>
{
	public:
		Subscriber_impl(const typename Client::ProtocolPtr_t protocol,
				const typename Client::String tag,
				const typename Client::String msgType,
				const typename Client::MsgCallback_t cb) :
				ReceiverInterface_impl<Client>(protocol, tag, msgType, cb)
		{
			ReceiverInterface_impl<Client>::_protocol->registerInterface(
					ReceiverInterface_impl<Client>::_tag, this);
		}

		~Subscriber_impl()
		{
			this->unsubscribe();
		}

		void receive(const typename Client::String &type,
				const typename Client::Value &msg,
				const typename Client::String &msgID);

		void unsubscribe()
		{
			ReceiverInterface_impl<Client>::_protocol->unregisterInterface(
					ReceiverInterface_impl<Client>::_tag, this);
		}
};

template<class Config>
class Client_impl
{
	public:
		typedef boost::shared_ptr<Client_impl> ClientPtr_t;
		typedef Config Config_t;
		typedef typename Config::String_type String;
		typedef typename Config::Value_type Value;
		typedef typename Config::Object_type Object;
		typedef typename Config::Binary_type Binary;
		typedef typename Config::Array_type Array;

		typedef ReceiverInterface_impl<Client_impl> Interface_t;
		typedef boost::function<void(const Value &msg)> MsgCallback_t;

		typedef ServiceClient_impl<Client_impl> ServiceClient_t;
		typedef Publisher_impl<Client_impl> Publisher_t;
		typedef Subscriber_impl<Client_impl> Subscriber_t;

		typedef boost::shared_ptr<ServiceClient_t> ServiceClientPtr_t;
		typedef boost::shared_ptr<Publisher_t> PublisherPtr_t;
		typedef boost::shared_ptr<Subscriber_t> SubscriberPtr_t;

		typedef boost::function<void(ClientPtr_t client)> ConnectCallback_t;

		typedef Protocol_impl<Client_impl> Protocol_t;
		typedef boost::shared_ptr<Protocol_t> ProtocolPtr_t;

	private:
		typedef websocketpp::client::handler::ptr _HandlerPtr_t;
		typedef websocketpp::client _WebsocketClient_t;
		typedef boost::shared_ptr<_WebsocketClient_t> _WebsocketClientPtr_t;
		typedef std::pair<String, Binary*> _BinaryIn_t;
		typedef std::vector<_BinaryIn_t> _BinaryInVector_t;

	public:
		Client_impl(const String userID, const String password,
				const String robotID) :
				_userID(userID), _password(password), _robotID(robotID), _connecting(
						false), _protocol(ProtocolPtr_t()), _endpoint(
						_WebsocketClientPtr_t())
		{
			if (!_initialized)
			{
				if (curl_global_init(CURL_GLOBAL_NOTHING))
					throw ClientException(
							"Can not initialize the curl library.");

				_initialized = true;
			}
		}

		void connect(const std::string &url, const ConnectCallback_t cb);
		void connected();
		void disconnect();

		void createContainer(const String &cTag) const;
		void destroyContainer(const String &cTag) const;

		void addNode(const String &cTag, const String &nTag, const String &pkg,
				const String &exe, const String &args, const String &name,
				const String &rosNamespace) const;
		void removeNode(const String &cTag, const String &nTag) const;

		void addParameter(const String &cTag, const String &name,
				const int value) const;
		void addParameter(const String &cTag, const String &name,
				const String &value) const;
		void addParameter(const String &cTag, const String &name,
				const double value) const;
		void addParameter(const String &cTag, const String &name,
				const bool value) const;
		void removeParameter(const String &cTag, const String &name) const;
		void
		addInterface(const String &eTag, const String &iTag,
				const interface_t iType, const String &iCls,
				const String &addr = "") const;
		void removeInterface(const String &eTag, const String &iTag) const;

		void addConnection(const String &iTag1, const String &iTag2) const;
		void addConnection(const String &eTag1, const String &iTag1,
				const String &eTag2, const String &iTag2) const;
		void removeConnection(const String &iTag1, const String &iTag2) const;
		void removeConnection(const String &eTag1, const String &iTag1,
				const String &eTag2, const String &iTag2) const;

		ServiceClientPtr_t service(const String &iTag, const String &srvType,
				const MsgCallback_t &cb);
		PublisherPtr_t publisher(const String &iTag, const String &msgType);
		SubscriberPtr_t subscriber(const String &iTag, const String &msgType,
				const MsgCallback_t &cb);

	private:
		void connectMaster(const std::string &url, std::string &robotURL,
				std::string &key);

		void addParameter(const String &cTag, const String &name,
				const Value &value) const;
		void configComponent(const String &type, const Value &component) const;
		void configConnection(const String &type, const String &iTag1,
				const String &iTag2) const;

		const String _userID;
		const String _password;
		const String _robotID;

		bool _connecting;
		ConnectCallback_t _cb;
		ProtocolPtr_t _protocol;
		boost::thread _connectedCB;
		_WebsocketClientPtr_t _endpoint;

		static bool _initialized;
};

typedef Client_impl<json_spirit::Config> Client;
typedef boost::shared_ptr<Client> ClientPtr;

typedef Client::MsgCallback_t MsgCallback;
typedef Client::ConnectCallback_t ConnectCallback;

typedef Client::ServiceClient_t ServiceClient;
typedef Client::Publisher_t Publisher;
typedef Client::Subscriber_t Subscriber;
typedef Client::ServiceClientPtr_t ServiceClientPtr;
typedef Client::PublisherPtr_t PublisherPtr;
typedef Client::SubscriberPtr_t SubscriberPtr;

typedef Client_impl<json_spirit::mConfig> mClient;
typedef boost::shared_ptr<mClient> mClientPtr;

typedef mClient::MsgCallback_t mMsgCallback;
typedef mClient::ConnectCallback_t mConnectCallback;
typedef mClient::ServiceClient_t mServiceClient;
typedef mClient::Publisher_t mPublisher;
typedef mClient::Subscriber_t mSubscriber;
typedef mClient::ServiceClientPtr_t mServiceClientPtr;
typedef mClient::PublisherPtr_t mPublisherPtr;
typedef mClient::SubscriberPtr_t mSubscriberPtr;

///////////////////////////////////////////////////////////////////////////////
/////  Implementation
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
////	Service_impl
template<class Client>
inline void ServiceClient_impl<Client>::call(const typename Client::Value &msg)
{
	call(msg, ReceiverInterface_impl<Client>::_cb);
}

template<class Client>
void ServiceClient_impl<Client>::call(const typename Client::Value &msg,
		const typename Client::MsgCallback_t &cb)
{
	typename Client::String uid = generateUUID<typename Client::String>();

	_callbacks.push_back(_CallbackRef_t(uid, cb));
	ReceiverInterface_impl<Client>::_protocol->send(
			ReceiverInterface_impl<Client>::_tag,
			ReceiverInterface_impl<Client>::_type, msg, uid);
}

template<class Client>
void ServiceClient_impl<Client>::receive(const typename Client::String &type,
		const typename Client::Value &msg, const typename Client::String &msgID)
{
	if (type != ReceiverInterface_impl<Client>::_type)
		throw ClientException("Received Service response with invalid type.");

	typename _CallbackRefVector_t::iterator it;

	for (it = _callbacks.begin(); it != _callbacks.end(); ++it)
		if (it->first == msgID)
		{
			(it->second).callback(msg);
			break;
		}

	if (it != _callbacks.end())
		_callbacks.erase(it);
	else
		throw ClientException(
				"Received Service response with invalid message ID.");
}

///////////////////////////////////////////////////////////////////////////////
////	Publisher_impl
template<class Client>
void Publisher_impl<Client>::publish(const typename Client::Value &msg)
{
	InterfaceBase_impl<Client>::_protocol->send(
			InterfaceBase_impl<Client>::_tag, InterfaceBase_impl<Client>::_type,
			msg, "nil");
}

///////////////////////////////////////////////////////////////////////////////
////	Subscriber_impl
template<class Client>
void Subscriber_impl<Client>::receive(const typename Client::String &type,
		const typename Client::Value &msg, const typename Client::String &msgID)
{
	if (type != ReceiverInterface_impl<Client>::_type)
		throw ClientException("Received Message with invalid type.");

	ReceiverInterface_impl<Client>::_cb(msg);
}

///////////////////////////////////////////////////////////////////////////////
////	Client_impl
template<class Config> bool Client_impl<Config>::_initialized;

struct curlMemory
{
		char *memory;
		size_t size;
};

size_t curlWriteCB(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct curlMemory *mem = (struct curlMemory*) userp;

	mem->memory = (char*) realloc(mem->memory, mem->size + realsize + 1);

	if (mem->memory == NULL) // out of memory!
		throw ClientException("Not enough memory.");

	memcpy(mem->memory + mem->size, contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0; // Terminate string with a zero byte

	return realsize;
}

template<class Config>
void Client_impl<Config>::connectMaster(const std::string &url,
		std::string &robotURL, std::string &key)
{
	curlMemory memory =
	{ 0 };
	int rc;
	long httprc;

	CURL *curl = curl_easy_init();
	char *err = new char[CURL_ERROR_SIZE];

	if (!curl)
		throw ClientException("Can not initialize the curl session.");

	if (!err)
		throw ClientException(
				"Not enough memory to allocated error message buffer.");

	// set the options for the next request
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCB);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &memory);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	// Perform the request
	rc = curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httprc);

	curl_easy_cleanup(curl); // always cleanup

	if (rc != CURLE_OK)
	{
		std::string msg = "Curl reported an error: ";
		msg = msg.append(std::string(err));

		if (memory.memory)
			free(memory.memory);
		delete err;

		throw ClientException(msg);
	}

	if (httprc != 200)
	{
		std::string msg = "Received an error from Master Manager: ";
		msg = msg.append(std::string(memory.memory));
		throw ClientException(msg);
	}

	// Process the received JSON string
	Value messageVal;
	_BinaryInVector_t binaries;
	json_spirit::read_string_or_throw(String(memory.memory), messageVal,
			binaries);

	if (!binaries.empty())
		throw ClientException(
				"Received message from Master Manager contains binary references.");

	if (messageVal.type() != json_spirit::obj_type)
		throw ClientException(
				"Received message from Master Manager has invalid JSON format.");

	Object message = messageVal.get_obj();

	Value keyVal = json_spirit::find_value<Object, String>(message, "key");
	Value urlVal = json_spirit::find_value<Object, String>(message, "url");
	Value curVal = json_spirit::find_value<Object, String>(message, "current");

	if (keyVal.type() != json_spirit::str_type
			|| urlVal.type() != json_spirit::str_type)
		throw ClientException(
				"Received message from Master Manager has invalid JSON format.");

	if (curVal.type() != json_spirit::null_type)
	{
		if (curVal.type() != json_spirit::str_type)
			throw ClientException(
					"Received message from Master Manager has invalid JSON format.");

		std::cout << "There is a newer client version (version: '"
				<< curVal.get_str() << "') available." << std::endl;
	}

	key = std::string(keyVal.get_str());
	robotURL = std::string(urlVal.get_str());
}

template<class Config>
void Client_impl<Config>::connect(const std::string &url,
		const ConnectCallback_t cb)
{
	// Check whether a connection attempt is valid.
	if (_connecting)
		throw ClientException("Already a connection attempt in progress.");

	if (_protocol && _endpoint)
		throw ClientException("Already a valid connection available.");

	_cb = cb;

#ifdef DEBUG
	std::cout << "Start connection with:" << std::endl;
	std::cout << "    userID: " << _userID << std::endl;
	std::cout << "    password: " << _password << std::endl;
	std::cout << "    robotID: " << _robotID << std::endl;
#endif

	// Make the initial HTTP request to the Master Manager
	_connecting = true;

	std::string robotURL;
	std::string key;

	std::ostringstream mStream;
	mStream << url << "?userID=" << _userID << "&password=" << _password
			<< "&robotID=" << _robotID << "&version=" << CLIENT_VERSION;
	connectMaster(mStream.str(), robotURL, key);

#ifdef DEBUG
	std::cout << "Received the following from Master Manager:" << std::endl;
	std::cout << "    url: " << robotURL << std::endl;
	std::cout << "    key: " << key << std::endl;
#endif

	_protocol = ProtocolPtr_t(new Protocol_t(ClientPtr_t(this)));
	_HandlerPtr_t robotHandler(_protocol);
	_endpoint = _WebsocketClientPtr_t(new _WebsocketClient_t(robotHandler));
	_connecting = false;

#ifdef DEBUG_WEBSOCKET
	_endpoint->alog().set_level(websocketpp::log::alevel::ALL);
	_endpoint->elog().set_level(websocketpp::log::elevel::ALL);
#else
	_endpoint->alog().unset_level(websocketpp::log::alevel::ALL);
	_endpoint->elog().unset_level(websocketpp::log::elevel::ALL);
#endif

	std::ostringstream rStream;
	rStream << robotURL << "?userID=" << _userID << "&robotID=" << _robotID
			<< "&key=" << key;
	_endpoint->connect(rStream.str());
	_endpoint->run(); // Blocking call
}

template<class Config>
void Client_impl<Config>::connected()
{
	_connectedCB = boost::thread(_cb, ClientPtr_t(this));
}

template<class Config>
void Client_impl<Config>::disconnect()
{
	_endpoint->reset();
	_protocol = ProtocolPtr_t();
	_endpoint = _WebsocketClientPtr_t();
}

template<class Config>
void Client_impl<Config>::createContainer(const String &cTag) const
{
	Object data;
	Config::add(data, "containerTag", cTag);

	_protocol->send(RCE_CREATE_CONTAINER, data);
}

template<class Config>
void Client_impl<Config>::destroyContainer(const String &cTag) const
{
	Object data;
	Config::add(data, "containerTag", cTag);

	_protocol->send(RCE_DESTROY_CONTAINER, data);
}

template<class Config>
void Client_impl<Config>::addNode(const String &cTag, const String &nTag,
		const String &pkg, const String &exe, const String &args,
		const String &name, const String &rosNamespace) const
{
	Object component;
	Config::add(component, "containerTag", cTag);
	Config::add(component, "nodeTag", nTag);
	Config::add(component, "pkg", pkg);
	Config::add(component, "exe", exe);
	Config::add(component, "args", args);
	Config::add(component, "name", name);
	Config::add(component, "namespace", rosNamespace);

	this->configComponent("addNodes", Value(component));
}

template<class Config>
void Client_impl<Config>::removeNode(const String &cTag,
		const String &nTag) const
{
	Object component;
	Config::add(component, "containerTag", cTag);
	Config::add(component, "nodeTag", nTag);

	this->configComponent("removeNodes", Value(component));
}

template<class Config>
inline void Client_impl<Config>::addParameter(const String &cTag,
		const String &name, const int value) const
{
	this->addParameter(cTag, name, Value(value));
}

template<class Config>
inline void Client_impl<Config>::addParameter(const String &cTag,
		const String &name, const String &value) const
{
	this->addParameter(cTag, name, Value(value));
}

template<class Config>
inline void Client_impl<Config>::addParameter(const String &cTag,
		const String &name, const double value) const
{
	this->addParameter(cTag, name, Value(value));
}

template<class Config>
inline void Client_impl<Config>::addParameter(const String &cTag,
		const String &name, const bool value) const
{
	this->addParameter(cTag, name, Value(value));
}

template<class Config>
void Client_impl<Config>::addParameter(const String &cTag, const String &name,
		const Value &value) const
{
	Object component;
	Config::add(component, "containerTag", cTag);
	Config::add(component, "name", name);
	Config::add(component, "value", value);

	this->configComponent("setParam", Value(component));
}

template<class Config>
void Client_impl<Config>::removeParameter(const String &cTag,
		const String &name) const
{
	Object component;
	Config::add(component, "containerTag", cTag);
	Config::add(component, "name", name);

	this->configComponent("deleteParam", Value(component));
}

template<class Config>
void Client_impl<Config>::addInterface(const String &eTag, const String &iTag,
		const interface_t iType, const String &iCls, const String &addr) const
{
	Object component;
	Config::add(component, "endpointTag", eTag);
	Config::add(component, "interfaceTag", iTag);

	switch (iType)
	{
	case INTERFACE_SERVICE_CLIENT:
		Config::add(component, "interfaceType", "ServiceInterface");
		break;

	case INTERFACE_SERVICE_PROVIDER:
		Config::add(component, "interfaceType", "ServiceProviderInterface");
		break;

	case INTERFACE_PUBLISHER:
		Config::add(component, "interfaceType", "PublisherInterface");
		break;

	case INTERFACE_SUBSCRIBER:
		Config::add(component, "interfaceType", "SubscriberInterface");
		break;

	case CONVERTER_SERVICE_CLIENT:
		Config::add(component, "interfaceType", "ServiceConverter");
		break;

	case CONVERTER_SERVICE_PROVIDER:
		Config::add(component, "interfaceType", "ServiceProviderConverter");
		break;

	case CONVERTER_PUBLISHER:
		Config::add(component, "interfaceType", "PublisherConverter");
		break;

	case CONVERTER_SUBSCRIBER:
		Config::add(component, "interfaceType", "SubscriberConverter");
		break;

	case FORWARDER_SERVICE_CLIENT:
		Config::add(component, "interfaceType", "ServiceForwarder");
		break;

	case FORWARDER_SERVICE_PROVIDER:
		Config::add(component, "interfaceType", "ServiceProviderForwarder");
		break;

	case FORWARDER_PUBLISHER:
		Config::add(component, "interfaceType", "PublisherForwarder");
		break;

	case FORWARDER_SUBSCRIBER:
		Config::add(component, "interfaceType", "SubscriberForwarder");
		break;
	}

	Config::add(component, "className", iCls);
	Config::add(component, "addr", addr);

	this->configComponent(String("addInterfaces"), Value(component));
}

template<class Config>
void Client_impl<Config>::removeInterface(const String &eTag,
		const String &iTag) const
{
	Object component;
	Config::add(component, "endpointTag", eTag);
	Config::add(component, "interfaceTag", iTag);
	this->configComponent("removeInterfaces", Value(component));
}

template<class Config>
void Client_impl<Config>::configComponent(const String &type,
		const Value &component) const
{
	Array array;
	array.push_back(component);

	Object data;
	Config::add(data, type, array);

	_protocol->send(RCE_CONFIGURE_COMPONENT, data);
}

template<class Config>
inline void Client_impl<Config>::addConnection(const String &eTag1,
		const String &iTag1, const String &eTag2, const String &iTag2) const
{
	this->configConnection("connect", eTag1 + "/" + iTag1, eTag2 + "/" + iTag2);
}

template<class Config>
inline void Client_impl<Config>::addConnection(const String &iTag1,
		const String &iTag2) const
{
	this->configConnection("connect", iTag1, iTag2);
}

template<class Config>
inline void Client_impl<Config>::removeConnection(const String &eTag1,
		const String &iTag1, const String &eTag2, const String &iTag2) const
{
	this->configConnection("disconnect", eTag1 + "/" + iTag1,
			eTag2 + "/" + iTag2);
}

template<class Config>
inline void Client_impl<Config>::removeConnection(const String &iTag1,
		const String &iTag2) const
{
	this->configConnection("disconnect", iTag1, iTag2);
}

template<class Config>
void Client_impl<Config>::configConnection(const String &type,
		const String &iTag1, const String &iTag2) const
{
	Object connection;
	Config::add(connection, "tagA", iTag1);
	Config::add(connection, "tagB", iTag2);

	Array array;
	array.push_back(connection);

	Object data;
	Config::add(data, type, array);

	_protocol->send(RCE_CONFIGURE_CONNECTION, data);
}

template<class Config>
typename Client_impl<Config>::ServiceClientPtr_t Client_impl<Config>::service(
		const String &iTag, const String &srvType, const MsgCallback_t &cb)
{
	return ServiceClientPtr_t(new ServiceClient_t(_protocol, iTag, srvType, cb));
}

template<class Config>
typename Client_impl<Config>::PublisherPtr_t Client_impl<Config>::publisher(
		const String &iTag, const String &msgType)
{
	return PublisherPtr_t(new Publisher_t(_protocol, iTag, msgType));
}

template<class Config>
typename Client_impl<Config>::SubscriberPtr_t Client_impl<Config>::subscriber(
		const String &iTag, const String &msgType, const MsgCallback_t &cb)
{
	return SubscriberPtr_t(new Subscriber_t(_protocol, iTag, msgType, cb));
}

} /* namespace rce */

#endif /* CLIENT_HXX_ */
