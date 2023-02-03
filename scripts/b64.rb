#!/bin/env ruby

 

require 'optparse'

 

$option = { :platform      => "lan966x_b0", }

 

OptionParser.new do |opts|

    opts.banner = "Usage: b64.rb [options]"

    opts.version = 0.1

    opts.on("-p", "--platform <platform>", "Build for given platform") do |p|

        $option[:platform] = p

    end

end.order!

 

puts "const #{$option[:platform]}_app = ["

STDIN.each_line do |line|

    line.chomp!()

    printf("    \"%s\",\n", line)

end

puts "]"