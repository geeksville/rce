#!/usr/bin/env python
# -*- coding: utf-8 -*-
#     
#     message.py
#     
#     This file is part of the RoboEarth Cloud Engine framework.
#     
#     This file was originally created for RoboEearth
#     http://www.roboearth.org/
#     
#     The research leading to these results has received funding from
#     the European Union Seventh Framework Programme FP7/2007-2013 under
#     grant agreement no248942 RoboEarth.
#     
#     Copyright 2012 RoboEarth
#     
#     Licensed under the Apache License, Version 2.0 (the "License");
#     you may not use this file except in compliance with the License.
#     You may obtain a copy of the License at
#     
#     http://www.apache.org/licenses/LICENSE-2.0
#     
#     Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.
#     
#     \author/s: Dominique Hunziker 
#     
#     

# zope specific imports
from zope.interface import implements

# Custom imports
from errors import InternalError, SerializationError
from util.interfaces import verifyClass, verifyObject, InterfaceError
from comm import types as msgTypes
from comm.message import Message
from comm import types
from comm.interfaces import IContentSerializer, IMessageProcessor
from core.interfaces import ISerializable, IMessenger


class ConnectDirectiveSerializer(object):
    """ Message content type to send an order to connect to other relay
        managers to a relay manager.
        
        The message is a list of (commID, IP address) tuples.
    """
    implements(IContentSerializer)
    
    IDENTIFIER = types.CONNECT
    
    def serialize(self, s, msg):
        s.addInt(len(msg))
        
        for element in msg:
            try:
                s.addElement(element[0])
                s.addElement(element[1])
            except KeyError as e:
                raise SerializationError('Could not serialize content '
                                         'ConnectDirective. Missing key: '
                                         '{0}'.format(e))
    
    def deserialize(self, s):
        return [(s.getElement(), s.getElement()) for _ in xrange(s.getInt())]


class ConnectDirectiveProcessor(object):
    """ Message processor for ConnectDirective messages. It is responsible for
        forwarding the message to the correct handler.
    """
    implements(IMessageProcessor)
    
    IDENTIFIER = types.CONNECT
    
    def __init__(self, manager):
        """ Initialize the connect directive processor.
        """
        self._manager = manager
    
    def processMessage(self, msg):
        self._manager.processRequest(msg.content)


class CommInfoSerializer(object):
    """ Message content type to send the communication ID of the relay manager
        to the container manager.
        
        The message content is the communication ID of the relay manager.
    """
    implements(IContentSerializer)
    
    IDENTIFIER = types.COMM_INFO
    
    def serialize(self, s, msg):
        s.addElement(msg)
    
    def deserialize(self, s):
        return s.getElement()


class CommInfoProcessor(object):
    """ Message processor for CommInfo messages. It is responsible for
        forwarding the message to the correct handler.
    """
    implements(IMessageProcessor)
    
    IDENTIFIER = types.COMM_INFO
    
    def __init__(self, manager):
        """ Initialize the communication information processor.
        """
        self._manager = manager
    
    def processMessage(self, msg):
        self._manager.registerRelayID(msg.content)


class RequestSerializer(object):
    """ Message content type to send a request.
        
        The fields are:
            user    User ID which is responsible for the request
            type    Identifier of request type
            args    Tuple containing all arguments necessary for the request
    """
    implements(IContentSerializer)
    
    IDENTIFIER = types.REQUEST
    
    def serialize(self, s, msg):
        if not isinstance(msg, dict):
            raise SerializationError('Content of the message Request '
                                     'has to be a dictionary.')
        
        try:
            s.addElement(msg['user'])
            s.addElement(msg['type'])
            s.addList(msg['args'])
        except KeyError as e:
            raise SerializationError('Could not serialize content of '
                                     'Request Message. Missing key: '
                                     '{0}'.format(e))
    
    def deserialize(self, s):
        return { 'user' : s.getElement(),
                 'type' : s.getElement(),
                 'args' : tuple(s.getList()) }


class CommandSerializer(object):
    """ Message content type to send a command class instance.
        
        The fields are:
            user    User ID which is responsible for the request
            cmd     The command which has to implement
                    core.interfaces.ISerializable
    """
    implements(IContentSerializer)
    
    IDENTIFIER = types.COMMAND
    
    def __init__(self):
        """ Initialize the command serializer.
        """
        self._cmdCls = {}
    
    def registerCommand(self, cmdList):
        """ Register a command class which should be used to (de-)serialize
            the command which should be added.
            
            @param cmdList:     List of command classes which should be
                                registered.
            @type  cmdList:     [ core.interfaces.ISerializable ]
            
            @raise:     util.interfaces.InterfaceError if the command class
                        does not implement core.interfaces.ISerializable.
        """
        for cmd in cmdList:
            verifyClass(ISerializable, cmd)
            self._cmdCls[cmd.IDENTIFIER] = cmd
    
    # TODO: Not really necessary; just for completeness
    #       Not called from anywhere at the moment.
    def unregisterCommand(self, cmd):
        """ Unregister a command class.
            
            @param cmd:     Command class which should be unregistered.
            @type  cmd:     core.interfaces.ISerializable
            
            @raise:     errors.InternalError if the command does not exist.
        """
        try:
            del self._cmdCls[cmd.IDENTIFIER]
        except KeyError:
            raise InternalError('Can not unregister a non-existent command.')
    
    def serialize(self, s, msg):
        try:
            s.addElement(msg['user'])
            cmd = msg['cmd']
        except KeyError as e:
            raise SerializationError('Could not serialize content of '
                                     'Command Message. Missing key: '
                                     '{0}'.format(e))
        
        try:
            verifyObject(ISerializable, cmd)
        except InterfaceError as e:
            raise SerializationError('Content of Command Message can not '
                                     'be serialized: {0}'.format(e))
        
        try:
            cls = self._cmdCls[cmd.IDENTIFIER]
        except KeyError:
            raise SerializationError('The object class is not registered.')
        
        if not isinstance(cmd, cls):
            raise SerializationError('The object is of invalid type.')
        
        s.addIdentifier(cmd.IDENTIFIER, 1)
        cmd.serialize(s)
    
    def deserialize(self, s):
        try:
            user = s.getElement()
            cls = self._cmdCls[s.getIdentifier(1)]
        except KeyError:
            raise SerializationError('The object class is not registered.')
        
        return { 'user' : user, 'cmd' : cls.deserialize(s) }


class TagSerializer(object):
    """ Message content type to send a tag with a type.
        
        The fields are:
            user    User ID which is responsible for the request
            tag     Tag/Name which should be sent
            type    Name to identify the type of tag
    """
    implements(IContentSerializer)
    
    IDENTIFIER = types.TAG
    
    def serialize(self, s, msg):
        if not isinstance(msg, dict):
            raise SerializationError('Content of the message Tag '
                                     'has to be a dictionary.')
        
        try:
            s.addElement(msg['user'])
            s.addElement(msg['tag'])
            s.addElement(msg['type'])
        except KeyError as e:
            raise SerializationError('Could not serialize content of '
                                     'Tag Message. Missing key: '
                                     '{0}'.format(e))
    
    def deserialize(self, s):
        return { 'user' : s.getElement(),
                 'tag'  : s.getElement(),
                 'type' : s.getElement() }


class RequestProcessor(object):
    """ Message processor for Request messages. It is responsible for
        forwarding the message to the correct handler.
    """
    implements(IMessageProcessor)
    
    IDENTIFIER = types.REQUEST
    
    def __init__(self, manager):
        """ Initialize the request processor.
        """
        super(RequestProcessor, self).__init__()
        
        self._manager = manager
    
    def processMessage(self, msg):
        self._manager.processRequest(msg.content)


class CommandProcessor(object):
    """ Message processor for Command messages. It is responsible for
        forwarding the message to the correct distributor.
    """
    implements(IMessageProcessor)
    
    IDENTIFIER = types.COMMAND
    
    def __init__(self, distributor):
        """ Initialize the command processor.
            
            @param distributor:     Used distributor for control messages.
            @type  distributor:     core.command.ControlDistributor
        """
        self._distributor = distributor
    
    def processMessage(self, msg):
        msg = msg.content
        self._distributor.processCommand(msg['user'], msg['cmd'])


class TagProcessor(object):
    """ Message processor for Tag messages. It is responsible for
        forwarding the message to the correct distributor.
    """
    implements(IMessageProcessor)
    
    IDENTIFIER = types.TAG
    
    def __init__(self, distributor):
        """ Initialize the tag processor.
            
            @param distributor:     Used distributor for control messages.
            @type  distributor:     core.command.ControlDistributor
        """
        self._distributor = distributor
    
    def processMessage(self, msg):
        msg = msg.content
        self._distributor.processTag(msg['user'], msg['type'], msg['tag'])


class ROSMsgSerializer(object):
    """ Message content type for a single ROS message.
        
        The fields are:
            user        User which is responsible for the message
            msg         Serialized ROS Message (str)
            destTag     Tag of destination interface
            srcTag      Tag of originating interface
            msgID       Unique ID to identify the message
    """
    implements(IContentSerializer)
    
    IDENTIFIER = types.ROS_MSG
    
    def serialize(self, s, msg):
        if not isinstance(msg, dict):
            raise SerializationError('Content of the message ROSMessage has '
                                     'to be a dictionary.')
        
        try:
            s.addElement(msg['msg'])
            s.addElement(msg['destTag'])
            s.addElement(msg['srcTag'])
            s.addElement(msg['msgID'])
            s.addElement(msg['user'])
        except KeyError as e:
            raise SerializationError('Could not serialize message of type '
                                     'ROSMessage. Missing key: {0}'.format(e))
    
    def deserialize(self, s):
        return { 'msg'     : s.getElement(),
                 'destTag' : s.getElement(),
                 'srcTag'  : s.getElement(),
                 'msgID'   : s.getElement(),
                 'user'    : s.getElement() }


class Messenger(object):
    """ MessageProcessor for ROS messages which is also responsible for sending
        the ROS messages originating from this endpoint.
    """
    implements(IMessageProcessor, IMessenger)
    
    IDENTIFIER = types.ROS_MSG
    
    def __init__(self, manager, commManager):
        """ Initialize the messenger.
            
            @param manager:     Manager to which the incoming messages will
                                be forwarded.
            @type  manager:     core.manager._InterfaceManager
        """
        self._manager = manager
        self._commManager = commManager
        self._commID = commManager.commID
    
    def send(self, userID, tag, commID, senderTag, msg, msgID):
        """ Send a message which was received from an endpoint to another
            interface.
            
            @param userID:      User ID of the interface owner.
            @type  userID:      str
            
            @param tag:         Tag of the interface at the destination.
            @type  tag:         str
            
            @param commID:      Communication ID of the destination.
            @type  commID:      str
            
            @param senderTag:   Tag of the interface who is sending the
                                message.
            @type  senderTag:   str
            
            @param msg:         ROS message in serialized form which should be
                                sent.
            @type  msg:         str
            
            @param msgID:       Identifier which is used to match a request to
                                a response.
            @type  msgID:       str
        """
        # First check if the message is for an interface in this endpoint
        if commID == self._commID:
            self._manager.received(userID, tag, commID, senderTag, msg, msgID)
            return
        
        msg = Message()
        msg.msgType = msgTypes.ROS_MSG
        msg.dest = commID
        msg.content = { 'msg'     : msg,
                        'destTag' : tag,
                        'srcTag'  : senderTag,
                        'msgID'   : msgID,
                        'user'    : userID }
        self._commManager.sendMessage(msg)
    
    def processMessage(self, msg):
        srcCommID = msg.origin
        msg = msg.content
        
        self._commManager.received(msg['user'], msg['destTag'], srcCommID,
                                   msg['srcTag'], msg['msg'], msg['msgID'])
