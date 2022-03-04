require 'capistrano/console'

# config valid for current version and patch releases of Capistrano
lock "~> 3.16.0"

set :application, "cgmembers-frame"
set :repo_url, "git@github.com:common-good/cgmembers-frame.git"

# Default branch is :master
# ask :branch, `git rev-parse --abbrev-ref HEAD`.chomp

# Default deploy_to directory is /var/www/my_app_name
set :deploy_to, "/home/new/cgmembers-frame"

# Default value for :format is :airbrussh.
# set :format, :airbrussh

# You can configure the Airbrussh format using :format_options.
# These are the defaults.
# set :format_options, command_output: true, log_file: "log/capistrano.log", color: :auto, truncate: :auto

# Default value for :pty is false
# set :pty, true

# Default value for :linked_files is []
# append :linked_files, "config/database.yml"
append :linked_files, "config/config.json"
append :linked_files, "config/phinx.json"

# Default value for linked_dirs is []
# append :linked_dirs, "log", "tmp/pids", "tmp/cache", "tmp/sockets", "public/system"
append :linked_dirs, "cgLogs", "cgPhotoTemp", "vendor", "cgmembers/.well-known"

# Default value for default_env is {}
# set :default_env, { path: "/opt/ruby/bin:$PATH" }

# Default value for local_user is ENV['USER']
# set :local_user, -> { `git config user.name`.chomp }

# Default value for keep_releases is 5
# set :keep_releases, 5

# Uncomment the following to require manually verifying the host key before first deploy.
# set :ssh_options, verify_host_key: :secure

set :tmp_dir, "/home/#{fetch(:local_user)}/tmp"

# Locals - this one sets the umask for the deployment
# SSHKit.config.umask = '002'
set :pty, true
