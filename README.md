# IrcFS

Intended for CI/BI IRC notifications.
IrcFS is released under the ISC (http://opensource.org/licenses/ISC) license.

# Usage

ircfs projectci@irc.freenode.net /tmp/freenode
ircfs -h

# Sending data
```

cat <<EOF > /tmp/freenode/\#\#my_project
This line will get sent to ##my_project
on the ol' irc.freenode.net. 
So will this line.
And so will this one.
And so on.
And so on.
And so on.
And so on.
EOF

```

# Reporting bugs

Create an Issue on github.
Or: fork, branch, fix, pull request.

# Current output of ircfs -h
```

Usage: ircfs [options] <connect line> <directory>                              
                                                                            
Where [options] can be one of:                                              
       -h              -- Show this output                                  
       -f              -- Run in foreground mode.                           
       -l <level>      -- Set log level (0:quiet, 7:noisy) (default: 5)    
       -b <burst>      -- Queue burst (default: 4)                         
       -w <seconds>    -- Wait <seconds> before reading file to channel.    
                          (The default value for this is: 20)               
       -D              -- Dry run, just print run time options and exit.    
                                                                            
<connect line> looks something like this: <nickname>@<server>[:port]        
       <nickname>      -- Defines the Nickname/Username/Realname to be used.
       <server>        -- The IRC host to connect to.                       
       <port>          -- The port to connect on. (default: 6667)             
                                                                            
The supplied <directory> should not exist. It will be created by IrcFS.     
                                                                            
Options may be placed anywhere, the only order that matters is that the     
connect line must come before the directory. Example:                       
                                                                            
               ircfs nk@irc.server.net:6668 /tmp/server -f                     
               ircfs nk@irc.server.net:6668 -f /tmp/server                     
               ircfs -f nk@irc.server.net:6668 /tmp/server                     
               ircfs nk@irc.server.net ~/irc/server -l 2                       

```
