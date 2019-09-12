# Copyright (C) 2015-2019, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

import os
import sys
import time
import xml.etree.ElementTree as ET
from datetime import datetime
from subprocess import DEVNULL, check_call, check_output
from typing import List


WAZUH_PATH = os.path.join('/', 'var', 'ossec')
WAZUH_CONF = os.path.join(WAZUH_PATH, 'etc', 'ossec.conf')
WAZUH_SOURCES = os.path.join('/', 'wazuh')
GEN_OSSEC = os.path.join(WAZUH_SOURCES, 'gen_ossec.sh')


class TimeMachine:
    """ Context manager that goes forward/back in time and comes back to real time once it finishes its instance
    """

    def __init__(self, timedelta):
        """ Saves time frame given by user

        :param timedelta: time frame
        :type timedelta: timedelta
        """
        self.time_delta = timedelta

    def __enter__(self):
        """ Calls travel_to_future function with saved timedelta as argument
        """
        self.travel_to_future(self.time_delta)

    def __exit__(self, exc_type, exc_value, exc_traceback):
        """ Calls travel_to_future again before exiting with a negative timedelta
        """
        self.travel_to_future(self.time_delta * -1)

    @staticmethod
    def _linux_set_time(time_):
        """ Changes date and time in a Linux system

        :param time_: new date and time to set
        :type time_: time
        """
        import subprocess
        import shlex

        subprocess.call(shlex.split("timedatectl set-ntp false"))
        subprocess.call(shlex.split("sudo date -s '%s'" % time_))

    @staticmethod
    def _win_set_time(time_):
        """ Changes date and time in a Windows system

        :param time_: new date and time to set
        :type time_: time
        """
        import os
        date_ = str(time_.date())
        time_ = str(time_.time()).split('.')
        time_ = time_[0]
        os.system('date ' + date_)
        os.system('time ' + time_)

    @staticmethod
    def travel_to_future(time_delta):
        """ Checks which system are we running this code in and calls its proper function

        :param time_delta: time frame we want to skip. It can have a negative value
        :type time_delta: timedelta
        """
        future = datetime.now() + time_delta
        if sys.platform == 'linux2' or sys.platform == 'linux':
            TimeMachine._linux_set_time(future.isoformat())
        elif sys.platform == 'win32':
            TimeMachine._win_set_time(future)


class TestEnvironment:
    """Class to prepare a custom configuration for a test."""

    def __init__(self, section: str, new_values: List, new_attributes: List,
                 checks: List = None) -> None:
        """Initialize TestEnvironment class.

        :param section: Section of 'ossec.conf' to edit
        :param new_values: List with dictionaries for replacing element values in a section
        :param new_attributes: Dictionary with the values of new attributes in a section
        :param checks: Dictionary with different checks for testing the environment
        """
        self.section = section
        self.new_values = new_values
        self.new_attributes = new_attributes
        self.checks = checks
        self.new_conf = set_section_configuration(self.section,
                                                  self.new_values,
                                                  self.new_attributes)

    def set_new_wazuh_conf(self):
        """Set the new Wazuh configuration. Wazuh will be restarted for applying it."""
        set_ossec_conf(self.new_conf)
        print("Restarting Wazuh...")
        command = os.path.join(WAZUH_PATH, 'bin/ossec-control')
        arguments = ['restart']
        check_call([command] + arguments, stdout=DEVNULL, stderr=DEVNULL)


def truncate_file(file_path):
    with open(file_path, 'w'):
        pass


def wait_for_condition(condition_checker, args=None, kwargs=None, timeout=-1):
    args = [] if args is None else args
    kwargs = {} if kwargs is None else kwargs
    timestep = 0.5
    max_iterations = timeout / timestep
    begin = time.time()
    iterations = 0
    while not condition_checker(*args, **kwargs):
        if timeout != -1 and iterations > max_iterations:
            raise TimeoutError()
        iterations += 1
        time.sleep(timestep)


def generate_ossec_conf(args: List = None) -> ET.ElementTree:
    """Generate a configuration file for Wazuh.

    :param args: Arguments for generating ossec.conf (install_type, distribution, version)
    :return: ElementTree with a new Wazuh configuration generated from 'gen_ossec.sh'
    """
    gen_ossec_args = args if args else ['conf', 'manager', 'rhel', '7']
    wazuh_config = check_output([GEN_OSSEC] + gen_ossec_args).decode(encoding='utf-8', errors='ignore')

    return ET.ElementTree(ET.fromstring(wazuh_config))


def get_ossec_conf() -> ET.ElementTree:
    """Get current 'ossec.conf' file.

    :return: ElemenTree with current Wazuh configuration
    """
    return ET.parse(WAZUH_CONF)


def set_ossec_conf(wazuh_conf: ET.ElementTree):
    """Set a new configuration in 'ossec.conf' file."""
    return wazuh_conf.write(WAZUH_CONF, encoding='utf-8')


def set_section_configuration(section: str = 'syscheck',
                              new_values: List = None,
                              new_attributes: List = None) -> ET.ElementTree:
    """Set a configuration in a section of Wazuh.

    :param wazuh_conf: XML with the Wazuh configuration (ossec.conf)
    :param new_values: List with dictionaries for settings elements
    :param new_attributes: dictionaries for setting attributes of elements
    :return: ElementTree with the custom Wazuh configuration
    """
    wazuh_conf = get_ossec_conf()
    section_conf = wazuh_conf.find('/'.join([section]))
    # clear section
    section_conf.clear()
    # insert elements
    for elem in new_values:
        for tag_name, new_value in elem.items():
            tag = ET.SubElement(section_conf, tag_name)
            tag.text = new_value
    # set attributes
    for elem in new_attributes:
        for tag_name, attr_list in elem.items():
            tag = wazuh_conf.find('/'.join([section, tag_name]))
            #tag = ET.SubElement(section_conf, tag_name)
            for attr in attr_list:
                attr_name, new_attr_value = list(attr.items())[0]
                tag.attrib[attr_name] = new_attr_value

    return wazuh_conf
