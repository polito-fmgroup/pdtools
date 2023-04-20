#! /usr/bin/perl

###PerlFile###########################################################
#
#  FileName	[ pdt1to2 ]
#  PackageName	[ pdtrav ]
#  Synopsis	[ automatic converter from pdtrav-1.* to pdtrav-2.0 ]
#  Description	[ ]
#
#  Author	[ Gianpiero Cabodi <cabodi@polito.it> ]
#  Revision	[ ]
#
######################################################################


#
# List of the known procedure conversions
#
%accumulateTable = (
# 
  "Ddi_BddNot",          1,
  "Ddi_BddSwapVars",     1,
  "Ddi_BddMakeMono",     1,
  "Ddi_BddAnd",          1,
  "Ddi_BddDiff",         1,
  "Ddi_BddNand",         1,
  "Ddi_BddOr",           1,
  "Ddi_BddNor",          1,
  "Ddi_BddXor",          1,
  "Ddi_BddXnor",         1,
  "Ddi_BddExist",        1,
  "Ddi_BddForall",       1,
  "Ddi_BddConstrain",    1,
  "Ddi_BddRestrict",     1,
  "Ddi_BddCproject",     1,
  "Ddi_BddAndExist",     1,
  "Ddi_BddIte",          1,
  "Ddi_VarSetNext",      1,
  "Ddi_VarSetAdd",       1,
  "Ddi_VarSetRemove",    1,
  "Ddi_VarSetDiff",      1,
  "Ddi_VarSetUnion",     1,
  "Ddi_VarSetIntersect", 1
);


######################################################################
#
# Global variables:
#

@sourceFiles = ();


# Return value for the whole program: 1 for an error condition
$returnValue = 0;

# Debug flag: 1 for debugging
$debug = 1;

#
# Get rid of all characters before the final / in the name of this program
#

$0 =~ s/^.*\///;

&convertFiles();

exit($returnValue);

##Sub################################################################
#
# Synopsis	[]
# Description	[]
# SideEffects	[]
#
######################################################################
sub convertFiles {

    $environment = "";		# Default environment

    #
    # Process command-line options
    #

    while ( $ARGV[0] =~ /^-/) {

        print "option: $ARGV[0] \n";
	$_ = shift(@ARGV);

    }

    #
    # Process the remaining arguments -- filenames
    #

    foreach $pathname (@ARGV) {

#	($dirname,$file) = &splitPathname($pathname);

        $readfile = $pathname . "\.1";
        $writefile = $pathname;

	#
	# Back up the old .c file
	#
	
	($dev,$ino,$mode) = stat $pathname;
	rename( $pathname, $readfile );
	
	open( FILE, $readfile );
	

 	&processFile(1);

	#
	# Regenerate the .c file, including the newly-extracted
	# prototypes in the automatic portion.  Set the access mode
	# to be the same as the original.
	#
	
	print "Writing $writefile\n";
	open( FILE, ">$writefile" );
	chmod( $mode, $writefile );
	print FILE $tmpFile;
#	print $tmpFile;

    }

}


##Sub################################################################
#
# Synopsis	[ Scan a file for substitutions ]
# Description	[ Scans a file for substitutions ]
# SideEffects	[  ]
#
######################################################################
sub processFile {

    local($shouldSave) = @_;

    $saveLines = $shouldSave;

    if ( $shouldSave ) {
      $tmpFile = "";
    }

    while ( <FILE> ) {


      # substring substitutions
#      s/ArrayFrom/ArrayMakeFrom/g;
#      s/\bDdi_VarMakeFrom/Ddi_VarFrom/g;
#      s/\bDdi_VarSetFrom/Ddi_VarSetMakeFrom/g;


      # special cases (changes in parameter list) 
#      s/BddLiteral\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)/BddMakeLiteral($2,1)/g;
#      s/BddComplLiteral\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)/BddMakeLiteral($2,0)/g;

      # word conversions (not converted if substrings)
#      s/\bDdi_BddReadPartNum\b/Ddi_BddPartNum/g;
#      s/\bDdi_BddReadPart\b/Ddi_BddPartRead/g;
#      s/\bDdi_BddExtractPart\b/Ddi_BddPartExtract/g;
#      s/\bDdi_BddFree\b/Ddi_Free/g;
#      s/\bDdi_BddArrayFree\b/Ddi_Free/g;
#      s/\bDdi_VarSetFree\b/Ddi_Free/g;
#      s/\bDdi_VarArrayFree\b/Ddi_Free/g;
#
      s/\bnew_node\(\s*(\w+)\s*,/new_node\(Expr_Ctl_$1_c,/g;

      $tmpFile .= $_;

    }

}

##Sub################################################################
#
# Synopsis	[ Split a pathname into a directory and file name. ]
# Description	[ ($dirname,$filename) = &splitPathname("/home/vis/fred.c")
#		  gives $dirname = "/home/vis/", $filename="fred.c" ]
# SideEffects	[]
#
######################################################################
sub splitPathname {
    $_ = $_[0];
    local($dirname,$filename) = /^(.*\/|)(.*)$/;
}

# Local Variables:
# mode: perl
# End:












