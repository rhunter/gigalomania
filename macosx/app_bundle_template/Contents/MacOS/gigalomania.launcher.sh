#!/bin/bash
env
macos_dir="${0%/*}"
resource_dir="${macos_dir%/MacOS}/Resources"
cd "$resource_dir"
exec "$macos_dir/gigalomania"

