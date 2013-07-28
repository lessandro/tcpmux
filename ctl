#!/bin/bash

supervisorctl -c supervisord.conf "$@"
