#!/usr/bin/env ruby
# Zajal Documentation Generator
# Ramsey Nasser, January 2011
require "rubygems"
require "mustache"

require "pp"
require "util/cdoc"

class String
  def titlecase
    self =~ /^\w/ ? sub(/\b\w/) { $&.upcase } : self
  end
end

module_doc = Mustache.new

source = open(ARGV[0]).read
init_func = source[/void Init.*\}/m]
source_body = source[source.index("#include")..source.index(init_func)-1].gsub(/\/\*.*\*\//, "")

# first big comment is the module description
module_doc[:description] = source[0, 1] == "/" ? source[/^\/\*.*?\*\//m].gsub(/^.?\*.?/, "") : ""

# pull declarations out of Init_* function
module_doc[:name] = module_name = init_func[/void Init_([^\(]+)/, 1]

module_var  = init_func[/([^\s=]+)\s*=\s*rb_define_module[^\(]*\([^"]+"#{module_doc[:name]}"/, 1]

# for documentation coverage
total_examples = 0
documented_methods = 0
total_methods = source.scan("rb_define_private_method").length

# extract private module methods
module_doc[:methods] = []
module_doc[:nodoc_methods] = []
init_func.scan(/rb_define_private_method\(#{module_var}[^"]+"([^"]+)"[^"]+RB_FUNC\(([^\)]+)/).each do |private_method|
  method_hash = {}
  method_hash[:name], c_name = private_method
  # diamonds are a hacker's best friend...
  comment = source_body.gsub(/ *(?:\/\*|\*\/)/, "◊").scan(/\s+◊([^◊]+?)◊\s+?VALUE\s+#{c_name}\b/m).first.to_s
  if comment.empty?
    module_doc[:nodoc_methods] << method_hash
    next
  end
  
  
  method_doc = CDoc.new comment

  method_hash[:description] = method_doc.description
  
  method_hash[:signatures] = method_doc.signatures.map do |arg_list|
    { :arg_list => arg_list.map { |a| a.name }.join(", "),
      :args => arg_list.map { |a| {:name => a.name, :description => a.description} } }
  end
  
  method_hash[:examples] = method_doc.examples.map { |e| {:content => e} }
  total_examples += 1 if method_hash[:examples].size > 0
  documented_methods += 1
  method_hash[:returns] = method_doc.returns.join(" ")[/^Returns (.*)/, 1]
  
  module_doc[:methods] << method_hash
end

stats = {}

stats[:total_methods] = total_methods
stats[:documented_methods] = documented_methods
stats[:documented_percent] = (documented_methods/total_methods.to_f * 100).round
stats[:exampled_methods] = total_examples
stats[:exampled_percent] = (total_examples/total_methods.to_f * 100).round

stats[:date] = Time.now.strftime "%F %R"
stats[:hash] = `git log -1 --pretty=format:%H`
stats[:short_hash] = `git log -1 --pretty=format:%h`

module_doc[:stats] = stats

puts module_doc.render DATA.read

__END__
# {{{name}}}
{{{description}}}

{{#methods}}
# {{{name}}}
{{{description}}}

**Syntax**

{{#signatures}}
`{{{name}}} {{{arg_list}}}`

{{#args}}
  * {{{name}}} - {{{description}}}
{{/args}}

{{/signatures}}
{{^signatures}}
`{{{name}}}`
{{/signatures}}

**Examples**

{{#examples}}
```ruby
{{{content}}}
```
{{/examples}}
{{^examples}}
*No examples provided*
{{/examples}}

**Returns** {{{returns}}}

{{/methods}}

{{#nodoc_methods}}
# {{{name}}}
*Not documented yet*

{{/nodoc_methods}}


---

{{#stats}}
Generated **{{{date}}}** against [{{{short_hash}}}](https://github.com/nasser/zajal/commit/{{{hash}}})

{{{documented_percent}}}% of methods documented, {{{exampled_percent}}}% given examples.
{{/stats}}