#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#       error.py
#       
#       This file is part of the RoboEarth Cloud Engine framework.
#       
#       This file was originally created for RoboEearth
#       http://www.roboearth.org/
#       
#       The research leading to these results has received funding from
#       the European Union Seventh Framework Programme FP7/2007-2013 under
#       grant agreement no248942 RoboEarth.
#       
#       Copyright 2012 RoboEarth
#       
#       Licensed under the Apache License, Version 2.0 (the "License");
#       you may not use this file except in compliance with the License.
#       You may obtain a copy of the License at
#       
#       http://www.apache.org/licenses/LICENSE-2.0
#       
#       Unless required by applicable law or agreed to in writing, software
#       distributed under the License is distributed on an "AS IS" BASIS,
#       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#       See the License for the specific language governing permissions and
#       limitations under the License.
#       
#       \author/s: Dominique Hunziker 
#       
#       

# twisted specific imports
from twisted.spread.pb import Error


class InternalError(Error):
    """ This class is used to signal an internal error.
    """


class InvalidRequest(Error):
    """ This class is used to signal an invalid request.
    """


class InvalidKey(Error):
    """ this class is used to signal an invalid key during the initialization
        of the ROS message connections.
    """


class MaxNumberExceeded(Error):
    """ Indicates that a quantity has exceeded an upper limit.
    """


class ConnectionError(Error):
    """ Error is raised when the connection failed unexpectedly.
    """


class DeadConnection(Exception):
    """ Error is raised to signal to the client protocol that the connection
        is dead.
    """


class AlreadyDead(Exception):
    """ Exception is raised when a death notifier callback is registered with
        an already dead object.
    """
