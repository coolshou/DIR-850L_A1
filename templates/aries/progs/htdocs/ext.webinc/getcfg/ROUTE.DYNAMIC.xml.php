<module>
	<service><?=$GETCFG_SVC?></service>
	<route>
		<dynamic><? echo "\n".dump(3, "/route/dynamic");?>		</dynamic>
	</route>
	<route6>
		<dynamic><? echo "\n".dump(3, "/route6/dynamic");?>      </dynamic>
	</route6>
</module>
