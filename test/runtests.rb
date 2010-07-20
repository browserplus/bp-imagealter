#!/usr/bin/env ruby

require 'optparse'
require 'optparse/time'
require 'ostruct'
require File.join(File.dirname(File.dirname(File.expand_path(__FILE__))),
                 "external/built/share/service_testing/bp_service_runner")
require 'uri'

class OptparseExample
  # Return a structure describing the options.
  def self.parse(args)
    # The options specified on the command line will be collected in *options*.
    # We set default values here.
    options = OpenStruct.new
    options.subdir = "build"
    options.substrpat = ""

    opts = OptionParser.new do |opts|
      opts.banner = "Usage: runtests.rb [options]"
      opts.separator ""
      opts.separator "Specific options:"

      # Optional argument; build output location.
      opts.on("-o", "--outputdir SUBDIR", String, "Specify subdir where output is located") do |subdir|
        options.subdir = subdir
      end

      # Optional argument; tests.
      opts.on("-t", "--test TEST", String, "Test name pattern") do |substrpat|
        options.substrpat = substrpat
      end

      opts.separator ""
      opts.separator "Common options:"

      # No argument, shows at tail.  This will print an options summary.
      # Try it and see!
      opts.on_tail("-h", "--help", "Show this message") do
        puts opts
        exit
      end

      # Another typical switch to print the version.
      opts.on_tail("--version", "Show version") do
        puts OptionParser::Version.join('.')
        exit
      end
    end

    opts.parse!(args)
    options
  end
end

# arguments are a string that must match the test name
options = OptparseExample.parse(ARGV)
substrpat = options.substrpat

rv = 0
curDir = File.dirname(__FILE__)
BrowserPlus.run("#{curDir}/../#{options.subdir}/ImageAlter") { |s|
  tests = 0
  successes = 0

  # now let's iterate through all of our tests
  Dir.glob(File.join(File.dirname(__FILE__), "cases", "*.json")).each do |f|
    next if substrpat && substrpat.length > 0 && !f.include?(substrpat)
    tests += 1 
    $stdout.write "#{File.basename(f, ".json")}: "
    $stdout.flush
    json = JSON.parse(File.read(f))
    # now let's change the 'file' param to a absolute URI
    p = File.join(File.dirname(__FILE__), "test_images", json["file"])
    p = File.expand_path(p)
    # now convert p into a file url
    json["file"] = ((p[0] == "/") ? "file://" : "file:///" ) + p

    took = Time.now
    r = s.transform(json)
    took = Time.now - took

    imgGot = nil
    begin
      imgGot = File.open(r['file'], "rb") { |oi| oi.read }
      wantImgPath = File.join(File.dirname(f),
                              File.basename(f, ".json") + ".out")
      raise "no output file for test!" if !File.exist? wantImgPath
      imgWant = File.open(wantImgPath, "rb") { |oi| oi.read }
      raise "output mismatch" if imgGot != imgWant
      # yay!  it worked!
      successes += 1
      puts "ok. (#{r['orig_width']}x#{r['orig_height']} -> #{r['width']}x#{r['height']} took #{took}s)"
    rescue => e
      err = e.to_s
      # for convenience, if the test fails, we'll *save* the output
      # image in xxx.got
      if imgGot != nil
        gotPath = File.join(File.dirname(f),
                            File.basename(f, ".json") + ".got")
        File.open(gotPath, "wb") { |oi| oi.write(imgGot) }
        err += " [left result in #{File.basename(gotPath)}]"
      end
      puts "fail (#{err} took #{took}s)"
    end
  end
  puts "#{successes}/#{tests} tests completed successfully"
  
  rv = successes == tests
}

exit rv

