<?php
/* $Id$ */
$tpl = new TemplatePower( template("grid_tree.tpl") );
$tpl->prepare();

$tpl->assign("self", "$self");

# Not as complicated as before. No depth-first-search, and
# we only show our immediate children.

# Publish past grids in our stack.
$ancestors = $gridstack;
# Take ourself off the end of the stack.
array_pop($ancestors);

if (count($ancestors)) {
   $tpl->newBlock("parentgrid");
   $parentgridtable = "";

   $parentgridstack = array();
   foreach ($ancestors as $g) {
      list($name, $link) = explode("@", $g);
      $parentgridstack[] = $g;
      $parentgridstack_url = rawurlencode(join(">", $parentgridstack));
      $parentgridtable .= "<tr><td align=center class=grid>".
         "<a href=\"$link?t=yes&amp;gw=back&amp;gs=$parentgridstack_url\">$name</a></td></tr>\n";
   }

   $tpl->assign("parents", $parentgridtable);
   $tpl->gotoBlock("_ROOT");
}

$gridtable="";

# Publish our children.
if ($n = count($grid))
   {
      $tpl->assign("n", $n);
      foreach ($grid as $source => $attrs)
         {
            if ($source == $self) continue;

            if (isset($grid[$source]['GRID']) and $grid[$source]['GRID'])
               {
                  # This child is a grid.
                  $url = $grid[$source]['AUTHORITY'] . "?t=yes&amp;gw=fwd&amp;gs=$gridstack_url";
                  $gridtable .= "<td class=grid><a href=\"$url\">$source</a></td>";
               }
            else
               {
                  # A cluster.
                  $url = "./?c=". rawurlencode($source) ."&amp;$get_metric_string";
                  $gridtable .= "<td><a href=\"$url\">$source</a></td>";
               }
         }
      $gridtable .= "</tr></table>";
   }

$tpl->assign("children", $gridtable);


$tpl->printToScreen();

?>
